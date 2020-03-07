/*
	TODO: inserire i minuti nelle estrazioni
*/

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

#include "../SHARED/utils.h"
#include "user_handle.h"


// massimo di thread gestibili dal sistema
#define MAX_THREADS 1000

// massimo di richieste di connessione gestibili contemporaneamente
#define MAX_LISTENER_TAIL 100

// tempo di risposta necessario prima di una disconnessione forzata
#define KEEPALIVE 60 * 30


typedef struct thread_param thread_param;

/* struttura parametro da passare ai thread che si occupa di comunicare con il client*/
struct thread_param{
	int sid; // id del socket relativo
	pthread_t tid; // id del thread
	users_signed* users;  // struttura che contiene gli utenti
	user *curr_user; // user relativo del thread impostato a NULL all'inizio
	char session_id[11]; // id di sessione
	gamble_list *g_list; // lista delle giocate attive
	int ip, index, n_try; // ip del client (ridondante), index indice del parametro nell'array
	ip_banned_struct *ips; // ips struttura che contiene gli ip bannati
	exit_number *ext; // struttura che contiene tutte le estrazioni
	time_t time_passed; // timestamp in secondi dell'ultima richiesta inviata dal client
};

/* struttura parametro da passare al thread che si occupa delle estrazioni */
typedef struct{
	pthread_t tid; // id del thread
	gamble_list *g_list; // puntatore alla lista delle giocate ative
	int extraction_time; // periodo di estrazioni
	exit_number *ext; // struttura che contiene tutte le estrazioni
} timer_param;

/* array che contiene i puntatori ai parametri dei thread gestiti */
thread_param** params;
/* numero di thread gestiti */
int N;
/* lock per la mutua esclusione sulle precedenti variabili */
pthread_mutex_t params_lock;

/* funzione che crea il socket che deve accettare le connessione del client*/
int create_listener(int _port){
	int listener_, counter = 0;
	struct sockaddr_in lis_addr; // Indirizzo client
	/* utilizzo socket bloccanti*/
	listener_ = socket(AF_INET, SOCK_STREAM, 0);
	lis_addr.sin_family = AF_INET;
	// Mi metto in ascolto su tutte le interfacce (indirizzi IP)
	lis_addr.sin_addr.s_addr = INADDR_ANY;
	lis_addr.sin_port = htons(_port);

	// provo ad effettuare il bind
	while(bind(listener_, (struct sockaddr*)& lis_addr, sizeof(lis_addr)) != 0 && counter < 10){
		printf("non riesco a connettere il socket di ascolto all'uscita\n");
		fflush(stdout);
		sleep(2);
		counter++;
	}

	// provo ad effettuare il listen
	while(listen(listener_, MAX_LISTENER_TAIL) != 0 && counter < 10){
		printf("non riesco ad azionare il socket di ascolto\n");
		fflush(stdout);
		sleep(2);
		counter++;
	}
	if(counter == 10)
		return 0;
	return listener_;
}

/* funzione che legge i paraemtri di ingresso */
int readInputParams(int _argc, char *_argv[], int *porta_, int *periodo_){
	int err = 0, temp;
	if(_argc == 1){
		printf("ERRORE numero porta parametro obbligatori\n");
		err |= 1;
	}
	if(_argc >= 2){
		*periodo_ = 300;
	    *porta_ = get_int_from_string(_argv[1], &temp);
	}
	if(_argc == 3){
		*periodo_ = get_int_from_string(_argv[2], &temp);
	}
	if(_argc > 3){
		printf("ERRORE: troppi parametri di ingresso\n");
		err |= -1;
	}
	return err;
}

/*
	le seguenti funzioni si occupano di interpretare i messaggi del client
	per far si che funzionino un messaggio del client deve essere incapsulato in un header protocol_header_client
	le funzioni sono della forma:
		-   void [nome_comando](thread_param* p, uint8_t* buffer, int b_size, uint8_t* res, protocol_header_server *header)
		-   thread_param* p: parametro passato al thread
		-   uint8_t* buffer: buffer di grandezza variabile inviato dal client
		-   int b_size: quantita di dati inseriti nel buffer
		-   uint8_t* res: buffer di risposta del server
		-   protocol_header_server *header: header di risposta, la funzione si dovrà occupare di riempire il valore data_size con la
											quantità di dati salvata in *res e il valore error_type in caso di errrori riscontrati
*/
void login(thread_param *_p, uint8_t *_params, int p_size, uint8_t *res_, struct protocol_header_server *header_){
	char *usr = (char*)_params, *psw = (char*)&_params[strlen((char*)_params) + 1];
	int i, flag, u_s;
	user *u;
	u_s = strlenbelow((char*)_params, p_size, &flag); // calcolo la lunghezza dello username
	if(flag){ // guardo se la formattazione è corretta: [username]\0[password]\0
		header_->error_type = BAD_FORMAT;
		return;
	}
	/* leggo lo username */
	usr = (char*)_params;
	_params = &_params[u_s + 1];
	strlenbelow((char*)_params, p_size - (u_s + 1), &flag);

	if(flag){ // guardo se la formattazione è corretta: [username]\0[password]\0
		header_->error_type = BAD_FORMAT;
		return;
	}

	/* leggo la password */
	psw = (char*)_params;

	u = correct_login(_p->users, usr, psw); // trovo l'utente relativo a tali credenziali
	header_->data_size = 0;

	/*
		guardo se l'utente è bannato, se l'utente è gia loggato con qualche credenziale
		o se l utente è gia connesso in un altro thread
	*/
	if(isBanned(_p->ips, _p->ip)){
		header_->error_type = BANNED;
	}else if(_p->curr_user != NULL){
		header_->error_type = ALREADY_LOGGED;
	}else if(u == NULL){// controllo che le credenziali immesse siano corrette
		header_->error_type = CREDENTIAL_NOT_CORRECT;
		_p->n_try++;
		/*
			3 tentativi 30 minuti di pausa
		*/
		if(_p->n_try == 3){
			insert_ip(_p->ips, _p->ip);
			_p->n_try = 0;
			header_->error_type = BANNED;
		}
	}else if(pthread_mutex_trylock(&u->lock) != 0){//controllo che non ci sia un altro thread con questo utente in tal caso invio errore
		header_->error_type = THREAD_LOGGED_IN;
	}else{
		/*
			calcolo il session id e lo restituisco
		*/
		char *ch = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		for(i = 0; i < 10; i++){
			_p->session_id[i] = ch[rand() % 62];
			res_[i] = _p->session_id[i];
		}
		_p->session_id[10] = '\0';
		_p->curr_user = u;
		res_[10] = '\0';

		header_->data_size = 11;
		header_->error_type = NO_ERROR;

		printf("-login dell'utente %s.\n", usr);

		_p->n_try = 0;
	}
}
void signup(thread_param *_p, uint8_t *_params, int p_size, uint8_t *res_, struct protocol_header_server *header_){
	char *usr, *psw;
	int u_s, p_s, flag;

	u_s = strlenbelow((char*)_params, p_size, &flag); // calcolo la lunghezza del username
	if(flag){ // guardo se la formattazione è corretta: [username]\0[password]\0
		header_->error_type = BAD_FORMAT;
		return;
	}
	usr = (char*)_params;
	p_s = strlenbelow((char*)&_params[u_s + 1], p_size - (u_s + 1), &flag);
	if(flag){ // guardo se la formattazione è corretta: [username]\0[password]\0
		header_->error_type = BAD_FORMAT;
		return;
	}
	psw = (char*)&_params[u_s + 1];

	if(u_s > 50 || p_s > 50 || u_s < 4 || p_s < 4){
		header_->error_type = CREDENTIAL_WRONG_SIZE;
	}else if(!insert_user(_p->users, usr, psw)){
		header_->error_type = USER_ALREADY_TAKEN;
	}else{
		printf("-iscrizione dell'utente %s.\n", usr);
		login(_p, _params, p_size, res_, header_);
	}
}
void invia_giocata(thread_param *_p, uint8_t *_params, int p_size, uint8_t *res_, struct protocol_header_server *header_){
	int i = 2, temp_counter = 0;
	gamble_data *g;

	if(_p->curr_user == NULL){ // se l'utente non è iscritto non puo attivare questa funzione
		header_->error_type = NOT_SIGNED;
		return;
	}

	/*
		creo e azzero una struttura per tenere la giocata
	*/
	g = malloc(sizeof(struct gamble_data));
	memset(g, 0, sizeof(struct gamble_data));

	g->u = _p->curr_user;
	g->ruote = _params[0] + (_params[1] << 8);

	/*
		leggo i numeri giocati
	*/
	memset(g->numbers, 0, sizeof(g->numbers[0]) * 10);
	while(i < p_size && _params[i] != 255 && i >= 1 && i <= 90){
		/*
			se sono superiori a 10 la richiesta è stata formattata male
		*/
		if(temp_counter >= 10){
			header_->error_type = BAD_FORMAT;
			free(g);
			return;
		}
		g->numbers[temp_counter] = _params[i];
		i++;
		temp_counter++;
	}
	/*
		secondo il protocollo l'ultimo elemento deve essere 255 senno è avvenuto un errore di formato
	*/
	if(_params[i] != 255){
		free(g);
		header_->error_type = BAD_FORMAT;
		return;
	}
	i++;

	temp_counter = 0;
	while(i < p_size){
		/*
			leggo i valori delle scommesse salvati
		*/
		memcpy(&g->values[temp_counter], &_params[i], sizeof(double));
		i += sizeof(double);
		temp_counter++;
	}

	/*
		se non ci sono stati errori e sonon arrivato fino a qui metto la giocata all'interno della lista
	*/
	header_->error_type = NO_ERROR;

	insert_gamble(_p->g_list, g, _p->ext);

	/*
		scrivo la riuscita del operazione
	*/
	printf("socket(%d) del user: %s giocata ricevuta, recap: \n\truote giocate: ", _p->sid, _p->curr_user->username);
	for(i = 0; i < N_RUOTE; i++)
		if((g->ruote & (1 << i)) != 0)
			printf("%s, ", ruota2str(i));
	printf("\n\tnumeri giocati:");
	for(i = 0; i < 10 && g->numbers[i] != 0; i++)
		printf("%d, ", g->numbers[i]);
	printf("\n\tscommesse giocate:");
	for(i = 0; i < 5 && g->numbers[i] != 0; i++){
		show_double(g->values[i]);
		printf(", ");
	}
	printf("\n");
}
void vedi_giocata(thread_param *_p, uint8_t *_params, int p_size, uint8_t *res_, struct protocol_header_server *header_){
	int tipo, i;
	gamble_data *list;
	printf("giocata inviata\n");fflush(stdout);
	if(p_size < 1){
		header_->error_type = BAD_FORMAT;
		return;
	}
	if(_p->curr_user == NULL){ // se l'utente non è iscritto non puo attivare questa funzione
		header_->error_type = NOT_SIGNED;
		return;
	}

	printf("giocata inviata\n");
	tipo = _params[0];
	if(tipo == 0){ // mostro le giocate non attive
		list = _p->curr_user->list;
		/* scorro tutta la lista delle giocate non attive*/
		while(list){
			// invio le ruote delle giocate 2 byte
			res_[header_->data_size++] = list->ruote;
			res_[header_->data_size++] = (list->ruote >> 8);

			// invio i numeri giocati con ultimo byte 255
			for(i = 0; i < 10 && list->numbers[i] != 0; i++)
				res_[header_->data_size++] = list->numbers[i];
			res_[header_->data_size++] = 255;
			i = i >= 5 ? 5 : i;

			// invio le scommesse effettuate come double
			memcpy(&res_[header_->data_size], list->values, i * sizeof(double));
			header_->data_size += i * sizeof(double);
			list = list->next;

		}
	}else{ // mostro solo le giocate attive
		pthread_mutex_lock(&_p->g_list->lock);

		list = _p->g_list->head;
		/* scorro tutta la lista delle giocate non attive*/
		while(list){
			if(list->u == _p->curr_user){
				// invio le ruote delle giocate 2 byte
				res_[header_->data_size++] = list->ruote;
				res_[header_->data_size++] = (list->ruote >> 8);

				// invio i numeri giocati con ultimo byte 255
				for(i = 0; i < 10 && list->numbers[i] != 0; i++)
					res_[header_->data_size++] = list->numbers[i];
				res_[header_->data_size++] = 255;
				i = i >= 5 ? 5 : i;

				// invio le scommesse effettuate come double
				memcpy(&res_[header_->data_size], list->values, i * sizeof(double));
				header_->data_size += i * sizeof(double);
			}
			list = list->next;
		}

		pthread_mutex_unlock(&_p->g_list->lock);
	}
	header_->error_type = NO_ERROR;

	printf("-invio giocata all utente %s\n", _p->curr_user->username);
}
void vedi_estrazione(thread_param *_p, uint8_t *_params, int p_size, uint8_t *res_, struct protocol_header_server *header_){
	int n = _params[0], i, k, j;
	extraction *ext;
	enum RUOTA r;

	if(_p->curr_user == NULL){ // se l'utente non è iscritto non puo attivare questa funzione
		header_->error_type = NOT_SIGNED;
		return;
	}

	/* se è stato inviato un solo parametro saranno specificate solo la quantita di estrazioni */
	if(p_size == 1)
		r = TUTTE;
	else
		r = _params[1];

	/*
		 leggo e invio cosi come sono state lette le estrazioni
		 |   n.estrazione sizeof(long) bytes  |
		 | ruota (1 byte) | 5 numeri (5 byte) |
		 | ruota (1 byte) | 5 numeri (5 byte) |
		                  .
		                  .
		                  .
		 |      255       | -> estrazione successiva
	*/
	pthread_mutex_lock(&_p->ext->lock);
	i = 0;

	memcpy(&res_[header_->data_size], &_p->ext->last, sizeof(long));
	header_->data_size += sizeof(long);

	for(ext = _p->ext->head; ext && i < n; ext = ext->next){
		for(k = (r == TUTTE ? 0 : r); k < (r == TUTTE ? N_RUOTE : (r + 1)); k++){
			res_[header_->data_size++] = k;
			for(j = 0; j < 5; j++){
				res_[header_->data_size++] = ext->numbers[k][j];
			}
		}
		res_[header_->data_size++] = 255;
		i++;
	}
	pthread_mutex_unlock(&_p->ext->lock);

	header_->error_type = NO_ERROR;
	printf("-invio estrazioni all utente %s\n", _p->curr_user->username);
}
void vedi_vincite(thread_param *_p, uint8_t *_params, int p_size, uint8_t *res_, struct protocol_header_server *header_){
	gamble_data *list;
	int i;

	if(_p->curr_user == NULL){ // se l'utente non è iscritto non puo attivare questa funzione
		header_->error_type = NOT_SIGNED;
		return;
	}

	list = _p->curr_user->list;
	while(list){
		/* invio a quale estrazioni si fa riferimento */
		memcpy(&res_[header_->data_size], &list->t, sizeof(long));
		header_->data_size += sizeof(long);

		/* 2 byte per la lista di ruote su cui è stata fatta la giocata */
		res_[header_->data_size++] = list->ruote;
		res_[header_->data_size++] = (list->ruote >> 8);


		/* invio i numeri giocati con 1 byte 255 come fine */
		for(i = 0; i < 10 && list->numbers[i] != 0; i++)
			res_[header_->data_size++] = list->numbers[i];
		res_[header_->data_size++] = 255;
		i = i >= 5 ? 5 : i;

		/* invio un ugual numero di giocate */
		memcpy(&res_[header_->data_size], list->values, i * sizeof(double));
		header_->data_size += i * sizeof(double);

		/* invio la quantita di numeri indovinati per ogni ruota giocata */
		for(i = 0; i < N_RUOTE; i++){
			if((list->ruote & (1 << i)) != 0)
				res_[header_->data_size++] = list->correct[i];
		}
		list = list->next;
	}
	header_->error_type = NO_ERROR;
	printf("-invio vincite all utente %s\n", _p->curr_user->username);
}
void esci(thread_param *_p, uint8_t *_params, int p_size, uint8_t *res_, struct protocol_header_server *header_){
	header_->error_type = NO_ERROR;
}

/* funzione eseguita dal thread che si occupa della gestione del timer */
void socket_disconnection(int _i, int _signal_type){
	int i = _i;

	if(_signal_type == SIGSTOP){
		printf("disconnessione volontaria del thread %d ", i);fflush(stdout);
	}if(_signal_type == SIGALRM){
		printf("disconnessione per timeout del thread %d ", i);fflush(stdout);
		pthread_cancel(params[i]->tid);
	}

	close(params[i]->sid);

	pthread_mutex_lock(&params_lock);


	if(params[i]->curr_user){
		printf("dell'utente : %s\n", params[i]->curr_user->username);
		pthread_mutex_unlock(&(params[i]->curr_user->lock));
	}

	printf("\n");


	N--;
	free(params[i]);

	params[i] = params[N];
	if(params[i])
		params[i]->index = i;

	pthread_mutex_unlock(&params_lock);

}


void disconnection_handle(int signal){}

void *handle_socket(void* _args){
	uint8_t *buf = malloc(MAX_DATA_SIZE * sizeof(uint8_t)), *res = malloc(MAX_DATA_SIZE * sizeof(uint8_t));
	int exit = 0;
	thread_param _p = *((thread_param*)_args);

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
		pthread_mutex_lock(&params_lock);
		((thread_param*)_args)->time_passed = time(NULL);
		((thread_param*)_args)->curr_user = _p.curr_user;
		pthread_mutex_unlock(&params_lock);

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
				case LOGIN:
					login(&_p, buf, header_recived.data_size, res, &header_send);
					break;
				case SIGNUP:
					signup(&_p, buf, header_recived.data_size, res, &header_send);
					break;
				case INVIA_GIOCATA:
					invia_giocata(&_p, buf, header_recived.data_size, res, &header_send);
					break;
				case VEDI_GIOCATA:
					vedi_giocata(&_p, buf, header_recived.data_size, res, &header_send);
					break;
				case VEDI_ESTRAZIONE:
			        vedi_estrazione(&_p, buf, header_recived.data_size, res, &header_send);
					break;
				case VEDI_VINCITE:
				    vedi_vincite(&_p, buf, header_recived.data_size, res, &header_send);
					break;
				case ESCI:
					esci(&_p, buf, header_recived.data_size, res, &header_send);
					exit = 1;
					break;
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
	if(_p.curr_user)
		printf("-disconnessione dell'utente %s.\n", _p.curr_user->username);


	free(buf);
	free(res);

	socket_disconnection(_p.index, SIGSTOP);

	pthread_exit(NULL);
	return NULL;
}

void *handle_timer(void* _args){
	timer_param _p = *((timer_param*) _args); // estraggo i parametri di ingresso
	int i;
	extraction *ext;
	srand(time(NULL));

	/* il thread satà sempre in esecuzione */
	while(1){
		sleep(_p.extraction_time); // aspetto la prossima estrazione

		pthread_mutex_lock(&_p.g_list->lock);

		ext = extract(_p.ext);
		evaluate(_p.g_list, ext);

		pthread_mutex_unlock(&_p.g_list->lock);
		/*
			vedo se ci sono connessioni con client non attive da un po' di tempo
			in tal caso le rimuovo
		*/
		pthread_mutex_unlock(&params_lock);
		for(i = 0; i < N; i++){
			if(params[i]->time_passed + KEEPALIVE < time(NULL)){
					pthread_mutex_unlock(&params_lock);
					socket_disconnection(i, SIGALRM);
					i--;
					pthread_mutex_lock(&params_lock);
			}
		}
		pthread_mutex_unlock(&params_lock);
	}

	pthread_exit(NULL);
	return NULL;
}


int main(int argc, char *argv[]){
	int periodo, porta;

	if(readInputParams(argc, argv, &porta, &periodo) == 0){

		/*
			le seguenti strutture salvano i dai che i threads andranno a modificare
			ricordo che gli utenti sono salvati con un hashtable (inserimento O(1) ricerca O(1))
			le giocate attive con una lista (inserimento O(1) eliminazione O(1))
			gli ip bannati con un hashtable con lista (inserimento O(1) ricerca O(1) ... la ricerca rispetto al tempo O(N) ma cio' è meno importante rispetto al nome dato che per il tempo è dedicato un intero thread)
			i numeri usciti con un insieme di matrici circolari (inserimento O(1) dopo un tot di estrazioni si vanno ad eliminare ... accettabile)
		*/
		users_signed *u = init_user_struct(1000000); // struttura che slava gli utenti iscritti il numero di utenti iscritti
		gamble_list *g = init_gamble_list(u); // struttura per le giocate attive
		ip_banned_struct *ips = init_ip_banned_struct(100000); // struttura per gli ip bannati
		exit_number *winn = init_exit_number(); // struttura per l'estrazione effettuate


		printf("avvio lotteria su porta %d, di periodo %d \n", porta, periodo);

		params = malloc(sizeof(thread_param*) * MAX_THREADS); // parametri per i thread che gestiscono la comunicazione client
		/*
			anche se timer e controller potrebbero essere implementati in un solo thread cio comporterebbe un inevitabile
			rallentamento nelle estrazioni cosa assolutamente non voluta dunque sono state separate le due funzioni
		*/
		timer_param timer; // parametro per la gestione delle estrazioni

		/*
			mutex per l'accesso in mutua esclusione sulla variabile N che identifica la quantità di threads attivi da modificare ogni volta che si
			aggiunge un client o un client esce
		*/
		pthread_mutex_init(&params_lock, NULL);

		struct sockaddr_in cl_addr; // Indirizzo client
		int listener; // Socket per l'ascolto
		int new_socket, res;
		int addrlen = sizeof(cl_addr);
		char str[INET_ADDRSTRLEN]; // stringa che conterra in caratteri l'indirizzo del nuovo client

		N = 0;

		signal(SIGPIPE, disconnection_handle);

		/* inizializzo i parametri del timer */
		timer.tid = 0;
		timer.g_list = g;
		timer.extraction_time = 60 * periodo;
		timer.ext = winn;



		listener = create_listener(porta);

		if(listener == 0){
			printf("non è stato possibile creare il server: %s\n", strerror(errno));
			return 0;
		}

		printf("server avviato con successo\n");

		res = pthread_create(&timer.tid, NULL, handle_timer, &timer); // avvio il timer

		while(1){
			if(N < MAX_THREADS){
				new_socket = accept(listener, (struct sockaddr *)&cl_addr, (socklen_t*)&addrlen);

				pthread_mutex_lock(&params_lock);

				/* creo i parametri per il thread N-esimo che gestisce un socket */
				params[N] = malloc(sizeof(thread_param));
				params[N]->sid = new_socket;
				params[N]->tid = N;
				params[N]->users = u;
				params[N]->curr_user = NULL;
				params[N]->g_list = g;
				params[N]->ip =  cl_addr.sin_addr.s_addr;
				params[N]->ips = ips;
				params[N]->n_try = 0;
				params[N]->ext = winn;
				params[N]->index = N;
				strcpy(params[N]->session_id, "0000000000");

				inet_ntop( AF_INET, &params[N]->ip, str, INET_ADDRSTRLEN );
				printf("nuova connessione da: %s, posizione nell'array dei thread: %d \n", str, N);

				res = pthread_create(&params[N]->tid, NULL, handle_socket, params[N]); // avvio il gestroe del client
				N++;

				pthread_mutex_unlock(&params_lock);

				if(res){
					printf("thread cannot be allocated..\n");
				}
			}
		}
	}
	return 0;
}
