#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <pthread.h>

#include "../shared/type.h"
#include "../shared/utils.h"
#include "../shared/networking.h"
#include "make_response.h"
#include "estrazioni.h"

// massimo num di thread (e dunque utenti connessi) gestibili dal sistema
#define MAX_THREADS 500

// massimo di richieste di connessione gestibili contemporaneamente
#define MAX_LISTENER_TAIL 100

// struttura passata al thread che esegue le estrazioni
typedef struct{
	pthread_t tid; // id del thread
	int periodo_estraz; // periodo di estrazioni
} timer_data;

// array che contiene i puntatori ai thread_slot (ogni thread che serve un client usa un thread_slot)
thread_slot** thread_slots;
// numero attuale di thread
int N_threads;
// lock per la mutua esclusione sulle precedenti variabili
pthread_mutex_t thread_slots_lock;

// variabili per controllare che i thread che servono i clienti
// non processino l'inserimento di nuove giocate fintanto che c'è un'estrazione in corso
// altrimenti la giocata potrebbe venire persa
int estrazione_in_corso;
pthread_mutex_t extr_lock;
pthread_cond_t end_of_extraction;


// restituisce la pos del primo thread_slot con il tid specificato
// se non lo trova, restituisce -1
int find_tid(pthread_t _tid){
	for (int i=0; i<N_threads; i++)
		if (thread_slots[i]->tid == _tid) return i;
	return -1;
}

// crea il socket che accetta le connessioni dai client
int create_listener(int _port){
	int listener_;
	struct sockaddr_in server_addr; // Indirizzo server
	listener_ = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	// Mi metto in ascolto su tutte le interfacce (indirizzi IP)
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(_port);

	// provo ad effettuare il bind
	if(bind(listener_, (struct sockaddr*) &server_addr, sizeof(server_addr)) != 0){
		printf("Bind del socket di ascolto all'uscita: Fallito\n");
		return 0;
	}

	// provo ad effettuare il listen
	if(listen(listener_, MAX_LISTENER_TAIL) != 0){
		printf("Attivazione del socket di ascolto: Fallito\n");
		return 0;
	}

	return listener_;
}

// legge porta e periodo indicati nel comando di avvio
// restituisce 1 in caso di errore, 0 altrimenti
int readInput(int _argc, char *_argv[], int *porta_, int *periodo_){
	if(_argc == 1){
		printf("ERRORE: parametri mancanti\n");
		return 1;
	}
	else if(_argc == 2){
		*periodo_ = 300; // 300 sec == 5 min
	    *porta_ = atoi(_argv[1]);
		return 0;
	}
	else if(_argc == 3){
		*periodo_ = atoi(_argv[2]);
		*porta_ = atoi(_argv[1]);
		return 0;
	}
	else if(_argc > 3){
		printf("ERRORE: troppi parametri di ingresso\n");
		return 1;
	}
	return 1;
}

// handler in caso di SIG_PIPE
void disconnection_handler(int signal){
 	pthread_t tid = pthread_self();
	pthread_mutex_lock(&thread_slots_lock);
	int i = find_tid(tid);
	if (i == -1) return; // il thread corrente non e' uno di quelli che servono client
	printf("Thread %d: E' venuta a mancare la connessione con il client\n", i);
	fflush(stdout);
	thread_slots[i]->exit = 1; // indica al thread che deve terminare
	pthread_mutex_unlock(&thread_slots_lock);
}

// libera le risorse associate al thread di index _index nell'array di thread_slots
void free_thread_slot(int _index){
	int _i = _index;
	printf("Thread %d: disconnessione dell'utente %s\n\n", _i, thread_slots[_i]->user);
	fflush(stdout);

	// rilascio delle risorse
	close(thread_slots[_i]->sid);
	free(thread_slots[_i]->req_buf);
	free(thread_slots[_i]->res_buf);

	free(thread_slots[_i]);
	thread_slots[_i] = NULL;
	N_threads --;
	if (N_threads == _i)
		return; // ho eliminato l'ultimo slot dell'array
	// in caso contrario, devo compattare l'array
	thread_slots[_i] = thread_slots[N_threads];
	thread_slots[_i]->index = _i;
}

// codice eseguito da ogni thread che serve una connessione utente
void *handle_socket(void* _args){
	thread_slot* thread_data = (thread_slot*)_args;
	enum ERROR err = NO_ERROR;
	enum COMMAND command = NO_COMMAND;

	// loop eseguito dal thread
	while(!thread_data->exit && err != BANNED && command!=ESCI){
		char* req_ptr = thread_data->req_buf; // buffer for request message
		char* res_ptr = thread_data->res_buf; // buffer for response message

		// mostra eventuale errore da ciclo precedente
		if (err != NO_ERROR){
			printf("Thread %d: ",thread_data->index);
			show_error(err);
		}
		err = NO_ERROR;

		err = get_msg(thread_data->sid, req_ptr);
		if (thread_data->exit == 1) err = DISCONNECTED; // ha ricevuto SIG_PIPE
		if (err != NO_ERROR) break;

		printf("Thread %d received:\n", thread_data->index);
		printf("%s\n", req_ptr);

		req_ptr = next_line(req_ptr); // salto la prima linea "CLIENT REQUEST"

		// leggo la SESSION ID
		char session_id[SESS_ID_SIZE];
		if (sscanf(req_ptr, "SESSION_ID: %s", session_id) == 0){
			send_error(thread_data->sid, BAD_REQUEST);
			continue;
		}
		req_ptr = next_line(req_ptr);

		if(strcmp(thread_data->session_id, session_id) != 0){
			send_error(thread_data->sid, WRONG_SESSID);
			break; //disconnetti
		}

		// leggo il comando ricevuto
		if (sscanf(req_ptr, "COMMAND: %d", (int*)&command) == 0)
			send_error(thread_data->sid, NO_COMMAND_FOUND);
		req_ptr = next_line(req_ptr);

		// compongo la risposta
		strcpy(res_ptr, "SERVER RESPONSE\n");
		res_ptr = next_line(res_ptr);
		// non posso processare la richiesta mentre e' in corso un estrazione
		pthread_mutex_lock(&extr_lock);
		if (estrazione_in_corso == 1)
			pthread_cond_wait(&end_of_extraction, &extr_lock);
		pthread_mutex_unlock(&extr_lock);
		err = make_response(thread_data, command, req_ptr, res_ptr);

		if (err == BANNED){
			send_error(thread_data->sid, err);
			printf("Tentativo di accesso da ip bannato %s\n", inet_ntoa(thread_data->ip));
			err = NO_ERROR;
			break;  //disconnetti
		}

		if (err == DISCONNECTED) break;

		// rispondo al client
		if(err != NO_ERROR)
			err = send_error(thread_data->sid, err);
		else
			err = send_msg(thread_data->sid, thread_data->res_buf);
	}
	//terminazione del thread
	//stampa eventuale errore
	if (err!=NO_ERROR){
		printf("Thread %d: ",thread_data->index);
		show_error(err);
	}
	//rilascia risorse
	pthread_mutex_lock(&thread_slots_lock);
	free_thread_slot(thread_data->index);
	pthread_mutex_unlock(&thread_slots_lock);
	pthread_exit(NULL);
	return NULL;
}

// codice eseguito dal thread che esegue le estrazioni
void *handle_timer(void* _args){
	timer_data* timer = (timer_data*)_args;

	while(1){ //ciclo infinto
		sleep(timer->periodo_estraz); // aspetto la prossima estrazione
		pthread_mutex_lock(&extr_lock);
		estrazione_in_corso = 1;
		make_estrazione();
		estrazione_in_corso = 0;
		pthread_cond_broadcast(&end_of_extraction);
		pthread_mutex_unlock(&extr_lock);
	}
	pthread_exit(NULL);
	return NULL;
}


int main(int argc, char *argv[]){
	int res; //tmp var per tenere il risultato di vari funzioni

	int periodo, porta;
	res = readInput(argc, argv, &porta, &periodo);
	if (res != 0) return 0;
	signal(SIGPIPE, disconnection_handler);

	printf("Avvio gioco del Lotto\nPorta: %d\nPeriodo: %d sec\n\n", porta, periodo);

	// array di thread_slot, numero di thread attivi, e mutex per queste due variabili
	pthread_mutex_init(&thread_slots_lock, NULL);
	thread_slots = (thread_slot**)malloc(MAX_THREADS*sizeof(thread_slot*));
	N_threads = 0;

	estrazione_in_corso = 0;
	pthread_mutex_init(&extr_lock, NULL);
	pthread_cond_init (&end_of_extraction, NULL);

	int listener; // Socket per l'ascolto
	listener = create_listener(porta);
	if(listener == 0){
		printf("Fallita creazione server: %s\n", strerror(errno));
		return 0;
	}

	printf("Server avviato con successo\n");

	timer_data timer; // parametro per il thread controllore
	timer.tid = 0;
	timer.periodo_estraz = periodo;
	pthread_create(&timer.tid, NULL, handle_timer, &timer); // avvio il timer

	// ciclo infinito
	while(1){
		if(N_threads < MAX_THREADS){
			struct sockaddr_in cl_addr; // Indirizzo client
			int addrlen = sizeof(cl_addr);
			int new_socket;
			new_socket = accept(listener, (struct sockaddr *)&cl_addr, (socklen_t*)&addrlen);

			pthread_mutex_lock(&thread_slots_lock);

			/* creo i parametri per il thread N-esimo che gestisce un socket */
			thread_slots[N_threads] = (thread_slot*)malloc(sizeof(thread_slot));
			thread_slots[N_threads]->sid = new_socket;
			thread_slots[N_threads]->req_buf = (char*)malloc(MAX_MSG_LENGTH); // buffer for request message
			thread_slots[N_threads]->res_buf = (char*)malloc(MAX_MSG_LENGTH); // buffer for response message
			thread_slots[N_threads]->ip =  cl_addr.sin_addr;
			thread_slots[N_threads]->n_try = 0;
			thread_slots[N_threads]->index = N_threads;
			strcpy(thread_slots[N_threads]->session_id, "0000000000");
			strcpy(thread_slots[N_threads]->user, "");
			thread_slots[N_threads]->exit = 0;

			char str[INET_ADDRSTRLEN];
			inet_ntop( AF_INET, &thread_slots[N_threads]->ip, str, INET_ADDRSTRLEN );
			printf("nuova connessione da: %s, posizione nell'array dei thread: %d \n", str, N_threads);

			 // avvio il gestore del client
			pthread_create(&thread_slots[N_threads]->tid, NULL, handle_socket, thread_slots[N_threads]);
			N_threads++;

			pthread_mutex_unlock(&thread_slots_lock);
		}
	}
}
