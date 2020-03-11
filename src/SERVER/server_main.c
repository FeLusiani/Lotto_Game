#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <pthread.h>

#include "../SHARED/utils.h"
#include "../SHARED/llist.h"


// massimo di thread gestibili dal sistema
#define MAX_THREADS 1000

// massimo di richieste di connessione gestibili contemporaneamente
#define MAX_LISTENER_TAIL 100

// tempo di risposta necessario prima di una disconnessione forzata
#define KEEPALIVE 60 * 30


typedef struct thread_slot thread_slot;

/* struttura parametro da passare ai thread che si occupa di comunicare con il client*/
struct thread_slot{
	int sid; // id del socket relativo
	pthread_t tid; // id del thread
	char session_id[11]; // id di sessione
	int ip, index, n_try; // ip del client (ridondante), index indice del parametro nell'array
	time_t time_passed; // timestamp in secondi dell'ultima richiesta inviata dal client
};

/* struttura parametro da passare al thread che si occupa delle estrazioni */
typedef struct{
	pthread_t tid; // id del thread
	int extraction_time; // periodo di estrazioni
} timer_data;

/* array che contiene i puntatori ai parametri dei thread gestiti */
thread_slot** thread_slots;
/* numero di thread gestiti */
int N;
/* lock per la mutua esclusione sulle precedenti variabili */
pthread_mutex_t thread_slots_lock;

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

// funzione che legge i paraemtri di ingresso
// restituisce 1 in caso di errore, 0 altrimenti
int readInput(int _argc, char *_argv[], int *porta_, int *periodo_){
	if(_argc == 1){
		printf("ERRORE: parametri mancanti\n");
		return 1;
	}
	else if(_argc == 2){
		*periodo_ = 300;
	    *porta_ = atoi(_argv[1]);
		return 0;
	}
	else if(_argc == 3){
		*periodo_ = atoi(_argv[2]);
	}
	else if(_argc > 3){
		printf("ERRORE: troppi parametri di ingresso\n");
		return 1;
	}
	return 1;
}


void disconnection_handle(int signal){}

void *handle_socket(void* _args){
	uint8_t *buf = malloc(MAX_DATA_SIZE * sizeof(uint8_t)), *res = malloc(MAX_DATA_SIZE * sizeof(uint8_t));
	int exit = 0;
	thread_slot _p = *((thread_slot*)_args);

	struct protocol_header_client header_recived;
	struct protocol_header_server header_send;

	while(!exit){
		/*
			PROTOCOLLO:
				- CLIENT ---> SERVER : invio istruzione
				- SERVER ---> CLIENT : ritorno errori ed evenuali dati
			CLIENT ---> SERVER:
				-   numero di bytes (4 byte) [obbligatorio]
				-   tipo di operazione (1 byte) [obbligatorio]
		        -   session_id (11 bytes) [obbligatorio]
		        -   data (dimensione variabile)
		    SERVER ---> CLIENT:
		    	-   numero di bytes (4 byte) [obbligatorio]
		    	-   tipo di errore (1 byte) [obbligatorio] (se non ci sono errori allora NO_ERROR)
		    	-   eventuali dati (dimensione variabile)
		*/
		pthread_mutex_lock(&thread_slots_lock);
		((thread_slot*)_args)->time_passed = time(NULL);
		pthread_mutex_unlock(&thread_slots_lock);

		recv(_p.sid, &header_recived, sizeof(struct protocol_header_client), 0);
		if(header_recived.data_size > 0)
			recv(_p.sid, buf, sizeof(uint8_t) * (header_recived.data_size), 0);

		/*
			osservo se il session id è sbagliato in tal caso qualcuno sta accedendo senza
			autorizzazione dunque invio un errroe ed esco altrimenti parso il comando ed
			eseguo determinate operazioni (se l'utente non si è ancora loggato il session_id
			"0000000000" )
		*/
		if(strcmp(_p.session_id, header_recived.session_id) != 0){
			header_send.error_type = WRONG_SESSID;
			header_send.data_size = 0;
			exit = 1;
		}else{
			header_send.error_type = NO_ERROR;
			header_send.data_size = 0;

			/*
				leggo quale comando è stato inviato
			*/
			switch(header_recived.command_type){
				default:
					header_send.data_size = 0;
					header_send.error_type = NO_COMMAND_FOUND;
					break;
			}
		}

		/* invio il buffer di risposta al client */
		send(_p.sid, (void*)&header_send, sizeof(struct protocol_header_server), 0);
		if(header_send.data_size > 0)
			send(_p.sid, (void*)res, header_send.data_size, 0);

		if(header_send.error_type == BANNED)
			break;
	}


	free(buf);
	free(res);

	pthread_exit(NULL);
	return NULL;
}

void *handle_timer(void* _args){
	return NULL;
}


int main(int argc, char *argv[]){
	int res; //tmp var per tenere il risultato di vari funzioni

	int periodo, porta;
	res = readInput(argc, argv, &porta, &periodo);
	if (res != 0) return 0;

	printf("Avvio gioco del Lotto\nPorta: %d\nPeriodo: %d\n\n", porta, periodo);

	// array di thread_slot, numero di thread attivi, e mutex per queste due variabili
	pthread_mutex_init(&thread_slots_lock, NULL);
	thread_slots = malloc(sizeof(thread_slot*) * MAX_THREADS);
	N = 0;

	timer_data timer; // parametro per il thread controllore
	timer.tid = 0;
	timer.extraction_time = 60 * periodo;


	int listener; // Socket per l'ascolto
	listener = create_listener(porta);
	if(listener == 0){
		printf("non è stato possibile creare il server: %s\n", strerror(errno));
		return 0;
	}

	printf("server avviato con successo\n");

	pthread_create(&timer.tid, NULL, handle_timer, &timer); // avvio il timer

	while(1){
		if(N < MAX_THREADS){
			struct sockaddr_in cl_addr; // Indirizzo client
			int addrlen = sizeof(cl_addr);
			int new_socket;
			new_socket = accept(listener, (struct sockaddr *)&cl_addr, (socklen_t*)&addrlen);

			pthread_mutex_lock(&thread_slots_lock);

			/* creo i parametri per il thread N-esimo che gestisce un socket */
			thread_slots[N] = malloc(sizeof(thread_slot));
			thread_slots[N]->sid = new_socket;
			thread_slots[N]->tid = N;
			thread_slots[N]->ip =  cl_addr.sin_addr.s_addr;
			thread_slots[N]->n_try = 0;
			thread_slots[N]->index = N;
			strcpy(thread_slots[N]->session_id, "0000000000");

			char str[INET_ADDRSTRLEN];
			inet_ntop( AF_INET, &thread_slots[N]->ip, str, INET_ADDRSTRLEN );
			printf("nuova connessione da: %s, posizione nell'array dei thread: %d \n", str, N);

			pthread_create(&thread_slots[N]->tid, NULL, handle_socket, thread_slots[N]); // avvio il gestroe del client
			N++;

			pthread_mutex_unlock(&thread_slots_lock);

		}
	}
}
