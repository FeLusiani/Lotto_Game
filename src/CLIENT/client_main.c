#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "message.h"

#define min(X, Y)  ((X) < (Y) ? (X) : (Y))

int not_reachable = 0;
/*
	qui sono inserite le funzioni di parsing dei comandi nel formato
	- ERROR nomecomando(char *str, int8_t *buffer, protocol_header_client *header);
	- char* str : equivale alla stringa scritta in input senza la sottostringa del comando (EX: input = "!login\0gino\0pino" str = "gino\0pino)
	- uint8_t* buffer: buffer che verra inviato al server
	- protocol_header_client *header : header del protocollo in cui la funzione dovrà settare il parametro data_size che definisce la grandezza del buffer
	- ritorna un SYNTAX_ERROR se il comando non è ben formattato, NO_SEND se non è necessario un invio al server per eseguire la funzione, NO_ERROR altrimenti
*/
enum ERROR help(char *_params, uint8_t *req_, struct protocol_header_client *header_){
	if(_params[0] == '\0'){
		show_basic_help(NO_COMMAND);
	}
	else{
		enum COMMAND c = str2command(_params);
		if(c == NO_COMMAND)
			printf("mi dispiace non esiste alcun comando con questo nome\n utilizza !help per vedere la lista di comandi\n");
		else
			show_basic_help(c);
	}
	return NO_SEND;
}

/*
	login invia nel buffer una stringa [username]\0[password]\0
*/
enum ERROR login(char *_params, uint8_t *req_, struct protocol_header_client *header_){
	int u_s, p_s;
	char *user, *pwd;

	user = _params;                       // variabile che contiene il nome utente, primo parametro
	pwd = &_params[strlen(_params) + 1]; // variabile che contiene la password, secondo parametro

	if(*user == '\0' || *pwd == '\0'){
		return SYNTAX_ERROR;
	}
	u_s = strlen(user) + 1;
	p_s = strlen(pwd) + 1;

	/* copio il nome utente e la password nel buffer e la grandezza del buffer nel header */
	header_->data_size = u_s + p_s;
	memcpy(&(req_[0]), user, u_s * sizeof(char));
	memcpy(&(req_[u_s]), pwd, p_s * sizeof(char));

	return NO_ERROR;
}
/*
	signup invia nel buffer una stringa [username]\0[password]\0
*/
enum ERROR signup(char *_params, uint8_t *req_, struct protocol_header_client *header_){
	int u_s, p_s;
	char *usr, *psw;

	if(_params[0] == '\0' || _params[strlen(_params) + 1] == '\0'){
		return SYNTAX_ERROR;
	}
	usr = _params;                       // variabile che contiene il nome utente, primo parametro
	psw = &_params[strlen(_params) + 1]; // variabile che contiene la password, secondo parametro
	u_s = strlen(usr) + 1;
	p_s = strlen(psw) + 1;

	if(p_s < 4 || u_s < 4 || p_s > 50 || u_s > 50)
		return CREDENTIAL_WRONG_SIZE;

	/* copio il nome utente e la password nel buffer e la grandezza del buffer nel header */
	header_->data_size = u_s + p_s;
	memcpy(&(req_[0]), usr, u_s * sizeof(char));
	memcpy(&(req_[u_s]), psw, p_s * sizeof(char));

	return NO_ERROR;
}
/*
	invia_giocata invia nel buffer una stringa |  ruote_giocate (2 byte) indirizzamento diretto  | numeri giocati (1 - 5 byte) | 255 (1 byte) | scommesse fatte((1 - 5) * sizeof(double)) |
*/
enum ERROR invia_giocata(char *_params, uint8_t *req_, struct protocol_header_client *header_){
	enum RUOTA r;
	int res, temp, num_counter = 0;
	double temp_double;
	uint16_t ruote = 0;

	if(strcmp(_params, "-r") != 0)
		return SYNTAX_ERROR;


	_params = &_params[strlen(_params) + 1]; // salto la sotto stringa già controllata "-r"
	/*
		estraggo le ruote dal loro nome dalla funzione str2ruota(char *str):

		se la sotto stringa corrente è "-n" ho finito di controllare le ruote
		se è "\0" ho finito le sottostringhe quindi lancio una SYNTAX_ERROR (non ci sono scommesse)
	*/
	while(strcmp(_params, "-n") && _params[0] != '\0'){
		r = str2ruota(_params);
		if(r == NO_RUOTA){// se la ruota scritta non esiste esco
			return SYNTAX_ERROR;
		}

		/*
			i bit del valore ruote indicano dove è stata inviata una scommessa
			- k-bit : 0 ruota r non giocata
			- k-bit : 1 ruota r giocata
			(gli enum sono interi)
		*/
		if(r == TUTTE){
			ruote |= ((1 << N_RUOTE) - 1);
		}else{
			ruote |= (1 << r);
		}

		_params = &_params[strlen(_params) + 1]; // passo alla sotto-stringa successiva
	}
	if(_params[0] == '\0')
		return SYNTAX_ERROR;

	/* essendo le route maggiori di 8 il valore ruote deve essere salvato in 2 byte */
	req_[header_->data_size++] = (ruote);
	req_[header_->data_size++] = (ruote >> 8);

	_params = &_params[strlen(_params) + 1]; // salto la sotto stringa già controllata "-n"
	/*
		leggo i numeri (al massimo 5 al minimo 1) sui quali scommettere
		i numeri devono essere compresi tra 0 e 90
		i controlli di loop sono uguali ai precedenti ma con sottostrigna "-i"
	*/
	while(strcmp(_params, "-i") && _params[0] != '\0'){
		num_counter++;
		res = get_int_from_string(_params, &temp); // parso il numero contenuto nella sottostringa
		if(_params[temp + 1] != '\0') // controllo che la sottostringa sia effettivamente un numero intero
			return SYNTAX_ERROR;
		if(res > 90 || res <= 0) // controllo che il numero sia compreso tra 0 e 90
			return SYNTAX_ERROR;
		if(num_counter > 10) // contorllo di aver inserito meno di 10 numeri
			return SYNTAX_ERROR;
		/* inseriso il numero (<90 dunque con dimensione 1 byte) all interno del buffer e procedo con la sottostringa successiva*/
		req_[header_->data_size++] = res;
		_params = &_params[strlen(_params) + 1];
	}
	req_[header_->data_size++] = 255; // il valore 255 mi serve come controllore di terminazione di numeri

	_params = &_params[strlen(_params) + 1]; // salto la sottostringa già controllata "-i"
	/*
		leggo il valore delle scommesse e le inserisco in una variabile double e la invio
	*/
	num_counter = num_counter >= 5 ? 5 : num_counter;
	while(_params[0] != '\0'){
		num_counter--;
		res = get_int_from_string(_params, &temp); // estraggo il numero fino al punto decimale (vedere il funzionamento di get_int_from_string)
		temp_double = res;
		/*
			se effettivamente il numero contiene una parte decimale la estraggo e calcolo la parte decimale e controllo che abbia meno di
			due cifre (non si può scommettere meno di un centesimo)
		*/
		if(_params[temp + 1] == '.'){

			_params = &_params[temp + 2];
			res = get_int_from_string(_params, &temp); // leggo la parte decimale
			if(_params[temp + 1] != '\0') // controllo che sia un numero intero
				return SYNTAX_ERROR;
			if(res >= 100) // controllo che sia minore di 100 dunque arrivi fino ai decimali
				return SYNTAX_ERROR;
			temp_double += (res < 10) ? ((double)res / 10) : ((double)res / 100);
		}else if(_params[temp + 1] != '\0'){
			return SYNTAX_ERROR;
		}
		/*
			inserisco il valore nel buffer aggiorno la quantita di elementi contenuti nel buffer e passo alla sottostringa successiva
		*/
		memcpy(&req_[header_->data_size], &temp_double, sizeof(double));
		header_->data_size += sizeof(double);
		_params = &_params[strlen(_params) + 1];
	}
	if(num_counter != 0) // il numero di scommesse deve essere compreso tra 1 e 5
		return SYNTAX_ERROR;

	return NO_ERROR;
}

/*
	vedi_giocata invia nel buffer 1 byte contenetnte 0 oppure 1
*/
enum ERROR vedi_giocata(char *_params, uint8_t *req_, struct protocol_header_client *header_){
	req_[header_->data_size++] = (_params[0] == '0') ? 0 : 1;
	return NO_ERROR;
}

/*
	vedi_estrazione invia nel buffer 1 byte contenente il numero di estrazioni precedenti quindi massimo 255 e 1 byte conitente la ruota
*/
enum ERROR vedi_estrazione(char *_params, uint8_t *req_, struct protocol_header_client *header_){
	int temp;

	req_[header_->data_size++] = get_int_from_string(_params, &temp);
	if(_params[strlen(_params) + 1] != '\0'){
		req_[header_->data_size++] = str2ruota(&_params[strlen(_params) + 1]);
	}

	return NO_ERROR;
}
enum ERROR vedi_vincite(char *_params, uint8_t *req_, struct protocol_header_client *header_){
	return NO_ERROR;
}
enum ERROR esci(char *_params, uint8_t *req_, struct protocol_header_client *header_){
	return NO_ERROR;
}

/*
	qui sono inserite le funzioni che mostrano a video eventuali dati
	inviato dal server nel formato:
	void [nomecomando]_ret(uint8_t* buf, protocol_header_server _header)
*/
void vedi_estrazione_ret(uint8_t *buf, struct protocol_header_server _header){
	int i, counter = 0, j = 0;
	long n_extraction;
	memcpy(&n_extraction, buf, sizeof(long));
	counter += sizeof(long);
	while(counter < _header.data_size){
		printf("\nESTRAZIONE: %ld\n", n_extraction - 1 - j);
		j++;
		while(buf[counter] != 255 && counter < _header.data_size){
			printf("%s : ", ruota2str(buf[counter++]));
			for(i = 0; i < 5; i++)
				printf("%d, ", buf[counter++]);
			printf("\n");
		}
		counter++;
	}
	printf("\n");
}

void vedi_giocata_ret(uint8_t *_buf, struct protocol_header_server _header){
	int i = 0, counter = 0, n_giocata = 1;
	uint16_t ruote;
	double temp_double;
	int N;
	char *gamble_type[5] = {"estratto", "ambo", "terno", "quartetto", "cinquina"};

	/*
		per ogni
	*/
	while(counter < _header.data_size){
		ruote = _buf[counter] + (_buf[counter + 1] << 8); // estraggo le ruote su cui è giocato (2 bytes)
		counter += 2;
		printf("%d) ruote: ", n_giocata);
		for(i = 0; i < N_RUOTE; i++){
			if((ruote & (1 << i)) != 0){
				printf("%s ", ruota2str(i));
			}
		}

		/*
			estraggo tutti i numeri giocati
		*/
		printf(" numeri giocati: ");
		for(i = 0; i < 10 && _buf[counter + i] != 255; i++){
			printf("%d, ", _buf[counter + i]);
		}
		counter += i + 1;
		N = i >= 5 ? 5 : i;

		/*
			estraggo le scommesse fatte (ricordo che sono double)
		*/
		for(i = 0; i < N; i++){
			memcpy(&temp_double, &_buf[counter], sizeof(double));
			counter += sizeof(double);
			printf("%s: ", gamble_type[i]);
			show_double(temp_double);
			printf(", ");
		}
		n_giocata++;
		printf("\n");
	}
}

void vedi_vincite_ret(uint8_t *_buf, struct protocol_header_server _header){
	int i = 0, j= 0, counter = 0, numbers[10], n_giocate, n_ruote, n_volta = 2;
	long t, preT = -1;
	uint16_t ruote;
	double values[5], tot_values[5] = {0,0,0,0,0}, temp_double;
	int N;
	double vincite[] = {11.23, 250, 4500, 120000, 6000000};
	char *gamble_type[5] = {"estratto", "ambo", "terno", "quartetto", "cinquina"};

	while(counter < _header.data_size){

		memcpy(&t, &_buf[counter], sizeof(long)); // estraggo il numero di estrazione
		counter += sizeof(long);

		/* se il numero di estraizone è diverso lo mostro */
		if(t != preT){
			printf("\n***************** estrazione (%ld) *****************\ngiocata 1:\n", t);
			n_giocate = 1;
			preT = t;
		}else{
			printf("giocata %d:\n", n_volta++);
			n_giocate = 1;
		}


		ruote = _buf[counter] + (_buf[counter + 1] << 8); // estraggo le ruote su cui è giocato
		counter += 2;
		/*  estraggo i numeri giocati e le quantita' di numeri giocati */
		memset(numbers ,0, 10 * sizeof(numbers[0]));
		for(i = 0; i < 10 && _buf[counter + i] != 255; i++){
			numbers[i] = _buf[counter + i];
		}
		counter += i + 1;
		N = i;

		/* estraggo tutte le scommesse sapendo la quantità di scommesse fate */
		memcpy(&values[0], &_buf[counter], min(N, 5) * sizeof(double));
		counter += min(N, 5) * sizeof(double);

		/*  calcolo la quantia totale di ruote giocate (hamming distance con il valore 0) */
		n_ruote = 0;
		for(i = 0; i < N_RUOTE; i++){
			n_ruote += ((ruote & (1 << i)) != 0);
		}

		/* per ogni ruota calcolo la scommessa */
		for(i = 0; i < N_RUOTE; i++){
			if((ruote & (1 << i)) != 0){ // osservo se la ruota è stata giocata
				printf("\t%d) ruota: ", n_giocate);
				printf("%s   ", ruota2str(i));
				for(j = 0; j < N; j++)
					printf("%d ", numbers[j]);
				printf(" >> ");

				/* _buf[counter] contiene la quantità di numeri indovinati... ricordo che il gioco è simmetrico non serve sapere quali numeri sono stati indovinati */
				for(j = 0; j < min(N, 5); j++){
					/*  osservo se sul seguente tipo di valore */
					if(values[j] != 0){
						temp_double = (j < _buf[counter]) ? values[j] : 0;
						if((int)temp_double != 0){
							temp_double /= n_ruote; // divido per le ruote su cui ho giocato
							temp_double *= vincite[j]; // moltiplico per il guadagno per tipo di giocata (AMBO, ESTRATTO, etc etc..)
							/* moltiplico per tutte le disposizioni fatte e divido per tutte quelle possibili */
							temp_double *= (double)disposizioni(_buf[counter], j + 1) / (double)disposizioni(N, j + 1);
							tot_values[j] += temp_double; // aggiungo la vincita a tutte le vincite
						}
						printf("%s ", gamble_type[j]);
						show_double(temp_double);
						printf("  ");
					}
				}
				counter++;
				printf("\n");
				n_giocate++;
			}
		}
	}

	printf("*********************************************\n\n");
	for(i = 0; i < 5; i++){
		printf("%s: ", gamble_type[i]);
		show_double(tot_values[i]);
		printf("\n");
	}
	printf("\n");
}

void send_to_server(int _sid, struct protocol_header_client *_p, uint8_t *_buff, enum ERROR *e){
	ssize_t res;

	res = send(_sid, (void*) _p, sizeof(struct protocol_header_client), 0);
	if(_p->data_size > 0 && res != -1)
		res = send(_sid, (void*) _buff, _p->data_size, 0);
	if(not_reachable == 1 || res == -1)
		*e = DISCONNECTED;
	else
		*e = NO_ERROR;
}

void recv_from_server(int _sid, struct protocol_header_server *_p, uint8_t *_buff, enum ERROR *e){
	ssize_t res;
	res = recv(_sid, (void*) _p, sizeof(struct protocol_header_server), 0);
	if(_p->data_size > 0 && res != -1)
		res = recv(_sid, (void*) _buff, _p->data_size, 0);
	if(not_reachable == 1 || res == -1)
		*e = DISCONNECTED;
	else
		*e = NO_ERROR;
}

void disconnection_handle(int signal){
	not_reachable = 1;
}

//	************************************************** MAIN *****************************************************************
int main (int argc, char *argv[]) {
	int sid, temp, porta, i, N;
	char *ip, input[1000], *str;
	uint8_t *buf = malloc(MAX_DATA_SIZE * sizeof(uint8_t));
	struct sockaddr_in sv_addr;

	signal(SIGPIPE, disconnection_handle);

	struct protocol_header_client header_send; // variabile che contiene l'header che il client invia al server
	struct protocol_header_server header_recived; // variabile che contiene l'header che il server ha inviato al client

	enum ERROR err;

	strcpy(header_send.session_id, "0000000000"); // il session id prima che venga configurato da login o signup è 0000000000

	if(argc == 3){
		/*	estraggo i parametri di inizio */
		ip = argv[1];
		porta = get_int_from_string(argv[2], &temp);

		/* Creazione socket */
		sid = socket(AF_INET, SOCK_STREAM, 0);

		/* Creazione indirizzo del server */
		memset(&sv_addr, 0, sizeof(sv_addr));
		sv_addr.sin_family = AF_INET ;
		sv_addr.sin_port = htons(porta);
		inet_pton(AF_INET, ip, &sv_addr.sin_addr);

		i = 0;
		while(i < 10 && connect(sid, (struct sockaddr*)&sv_addr, sizeof(sv_addr)) != 0){
			sleep(1);
			i++;
		}

		if(i == 10){
			printf("impossibile connettersi al server: %s\n", strerror(errno));
			return 0;
		}


		show_basic_help(NO_COMMAND);

		header_send.command_type = NO_COMMAND;
		while(header_send.command_type != ESCI && err != BANNED && not_reachable == 0){

			printf("$");
			i = 0;
			/* estraggo la line di comando */
			do{
				scanf("%c", &input[i]);
				if(i >= 1000)
					break;
			}while(input[i++] != '\n');
			input[i - 1] = '\0';
			N = i - 1;
			err = NO_ERROR;
			/* inizio il parser eliminando gli spazi sostituendoli con \0 garantisce di avere piu strighe consecutive nello stesso array con terminazione \0\0 */
			for(i = 0; i < N; i++){
				if(input[i] == ' ' && input[i + 1] == ' '){
					printf("troppi spazi\n");
					err = GENERIC_ERROR;
					break;
				}
				input[i] = (input[i] == ' ') ? '\0' : input[i];
			}
			input[N + 1] = '\0';
			str = input;

			if(err == NO_ERROR){
				/* estraggo il comando come prima sottostringa */
				header_send.command_type = str2command(str);
				str = &str[strlen(str) + 1];

				if(header_send.command_type == NO_COMMAND){ // comando non riconosciuto
					printf("comando non riconosciuto\n");
				}else{
					err = NO_SEND;

					header_send.data_size = 0;
					/*
						per ogni istruzione definisco un parser per tale istruzione nella forma
							- ERROR nomecomando(char *str, int8_t *buffer, protocol_header_client *header);
							- char* str : equivale alla stringa scritta in input senza la sottostringa del comando (EX: input = "!login\0gino\0pino" str = "gino\0pino)
							- uint8_t* buffer: buffer che verra inviato al server
							- protocol_header_client *header : header del protocollo in cui la funzione dovrà settare il parametro data_size che definisce la grandezza del buffer
							- ritorna un SYNTAX_ERROR se il comando non è ben formattato, NO_SEND se non è necessario un invio al server per eseguire la funzione, NO_ERROR altrimenti
					*/
					switch(header_send.command_type){
						case HELP:
							err = help(str, buf, &header_send);
							break;
						case LOGIN:
							err = login(str, buf, &header_send);
							break;
						case SIGNUP:
							err = signup(str, buf, &header_send);
							break;
						case INVIA_GIOCATA:
							err = invia_giocata(str, buf, &header_send);
							break;
						case VEDI_GIOCATA:
							err = vedi_giocata(str, buf, &header_send);
							break;
						case VEDI_ESTRAZIONE:
							err = vedi_estrazione(str, buf, &header_send);
							break;
						case VEDI_VINCITE:
							err = vedi_vincite(str, buf, &header_send);
							break;
						case ESCI:
							err = esci(str, buf, &header_send);
							break;
					}
					if(err == NO_ERROR){
						/* invio prima l'header e poi incapsulo il buffer se la grandezza di esso è maggiore di 0 */
						send_to_server(sid, &header_send, buf, &err);

						/* ricevo prima l'header e poi estraggo il buffer se la grandezza di esso è maggiore di 0 */
						if(err == NO_ERROR)
							recv_from_server(sid, &header_recived, buf, &err);

						if(err != NO_ERROR)
							header_recived.error_type = err;
						show_error((enum ERROR)header_recived.error_type); // mostro a video eventuali errori
						err = header_recived.error_type;
						/*
							se non ci sono errori, in base all comando inviato al server, mostro a video eventuali dati
							oppure salvo il valore session_id inviato dl server dopo una login / signup
						*/
						if(header_recived.error_type == NO_ERROR){
							switch(header_send.command_type){
								case INVIA_GIOCATA:
									printf("invio riuscito\n");
								break;
								case LOGIN:
									strcpy(header_send.session_id, (char*)buf);
									break;
								case SIGNUP:
									strcpy(header_send.session_id, (char*)buf);
									printf("iscrizione avvenuta con successo\n");
									break;
								case VEDI_GIOCATA:
									vedi_giocata_ret(buf, header_recived);
									break;
								case VEDI_ESTRAZIONE:
									vedi_estrazione_ret(buf, header_recived);
									break;
								case VEDI_VINCITE:
									vedi_vincite_ret(buf, header_recived);
									break;
								default:
									break;
							}
						}
					}else if(err != NO_SEND){
						show_error(err);
					}
				}
			}
		}
		close(sid);
	}
	if(err == NO_ERROR)
		printf("disconnessione avvenuta con successo\n");
	free(buf);

	return 0;
}
