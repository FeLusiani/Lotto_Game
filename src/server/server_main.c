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

#include "../SHARED/type.h"
#include "../SHARED/utils.h"
#include "../SHARED/networking.h"
#include "make_response.h"
#include "estrazioni.h"


// massimo num di thread (e dunque utenti connessi) gestibili dal sistema
#define MAX_THREADS 500

// massimo di richieste di connessione gestibili contemporaneamente
#define MAX_LISTENER_TAIL 100

// tempo di risposta necessario prima di una disconnessione forzata
#define KEEPALIVE 60 * 30


// struttura passata al thread controllore
typedef struct{
	pthread_t tid; // id del thread
	int periodo_estraz; // periodo di estrazioni
} timer_data;

// array che contiene i puntatori ai thread_slot (ogni thread che serve un client usa un thread_slot)
thread_slot** thread_slots;
// numero attuale di thread
int N_threads;
/* lock per la mutua esclusione sulle precedenti variabili */
pthread_mutex_t thread_slots_lock;

// restituisce la pos del primo thread_slot con il tid specificato
// se non lo trova, restituisce -1
int find_tid(pthread_t _tid){
	for (int i=0; i<N_threads; i++)
		if (thread_slots[i]->tid == _tid) return i;
	return -1;
}

// la funzione che crea il socket che accetta le connessioni dai client
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

// funzione che legge i parametri di ingresso
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

//
void disconnection_handler(int signal){
 	pthread_t tid = pthread_self();
	pthread_mutex_lock(&thread_slots_lock);
	int i = find_tid(tid);
	if (i == -1) return; // il thread corrente non e' uno di quelli che servono client
	printf("Thread %d: E' venuta a mancare la connessione con il client\n", i);
	fflush(stdout);
	thread_slots[i]->exit = 1;
	pthread_mutex_unlock(&thread_slots_lock);
}

void free_thread_slot(int _index, int _signal){
	int _i = _index;
	if(_signal == SIGSTOP){
		printf("Thread %d: disconnessione volontaria ", _i);
		fflush(stdout);
	}
	if(_signal == SIGALRM){
		printf("Thread %d: disconnessione per timeout ", _i);
		fflush(stdout);
		pthread_cancel(thread_slots[_i]->tid);
	}
	printf("dell'utente [%s]\n\n", thread_slots[_i]->user);

	// rilascio delle risorse
	close(thread_slots[_i]->tid);
	free(thread_slots[_i]->req_buf);
	free(thread_slots[_i]->res_buf);

	free(thread_slots[_i]);
	thread_slots[_i] = NULL;
	N_threads --;
	if (N_threads == _i)
		return; // ho eliminato l'ultimo slot dell'array
	// in caso contrario, devo compattare
	thread_slots[_i] = thread_slots[N_threads];
	thread_slots[_i]->index = _i;
}

void *handle_socket(void* _args){
	thread_slot* thread_data = (thread_slot*)_args;
	enum ERROR err = NO_ERROR;


	while(!thread_data->exit && err != BANNED){
		char* req_ptr = thread_data->req_buf; // buffer for request message
		char* res_ptr = thread_data->res_buf; // buffer for response message

		// mostra eventuale errore da ciclo precedente
		if (err != NO_ERROR)
			show_error(err);
		err = NO_ERROR;
		thread_data->last_request = time(NULL);

		err = get_msg(thread_data->sid, req_ptr);
		if (thread_data->exit == 1) err = DISCONNECTED;
		if (err != NO_ERROR) break;

		req_ptr = next_line(req_ptr); // salto la prima linea "CLIENT REQUEST"
		fflush(stdout);

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
		enum COMMAND command;
		if (sscanf(req_ptr, "COMMAND: %d", (int*)&command) == 0)
			send_error(thread_data->sid, NO_COMMAND_FOUND);
		req_ptr = next_line(req_ptr);

		if (command == ESCI) break;

		// compongo la risposta
		strcpy(res_ptr, "SERVER RESPONSE\n");
		res_ptr = next_line(res_ptr);
		err = make_response(thread_data, command, req_ptr, res_ptr);

		if (err == DISCONNECTED) break;


		// rispondo al client
		if(err != NO_ERROR)
			err = send_error(thread_data->sid, err);
		else
			err = send_msg(thread_data->sid, thread_data->res_buf);
	}

	if (err!=NO_ERROR) show_error(err);

	pthread_mutex_lock(&thread_slots_lock);
	free_thread_slot(thread_data->index, SIGSTOP);
	pthread_mutex_unlock(&thread_slots_lock);
	pthread_exit(NULL);
	return NULL;
}


void *handle_timer(void* _args){
	timer_data* timer = (timer_data*)_args;

	while(1){ //ciclo infinto
		sleep(timer->periodo_estraz); // aspetto la prossima estrazione
		make_estrazione();
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
			thread_slots[N_threads]->last_request = time(NULL);
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
