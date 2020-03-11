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

#include "../SHARED/utils.h"
#include "message.h"

#define min(X, Y)  ((X) < (Y) ? (X) : (Y))

int not_reachable = 0;
void disconnection_handler(int signal){
	not_reachable = 1;
}

// le seguenti funzioni sono utilizzate per eseguire i comandi del client
// ogni funzione si occupa di scrivere il messaggio da scrivere al server (header e buffer)
// Se non ci sono errori, restituisce NO_ERROR
enum ERROR help(char *_params, uint8_t *msg_buf, struct client_msg_header *header_){
	header_->buff_size = 0;
	if(_params[0] == '\0'){
		show_help(NO_COMMAND);
		return NO_ERROR;
	}
	enum COMMAND c = str2command(_params);
	if(c == NO_COMMAND)
		printf("Non esiste alcun comando con questo nome\nUsa !help per vedere la lista dei comandi\n");
	else
		show_help(c);

	return NO_ERROR;
}

enum ERROR signup(char* _params, uint8_t* msg_buf_, struct client_msg_header* header_){
	header_->buff_size = 0;
	int u_length, p_length;
	char *user, *pwd;

	// leggo i parametri
	user = _params;
	pwd = &_params[strlen(_params) + 1];

	if(*user == '\0' || *pwd == '\0'){
		return SYNTAX_ERROR;
	}
	u_length = strlen(user) + 1;
	p_length = strlen(pwd) + 1;

	// credentials size tra 4 e 50 char
	if(p_length < 5 || u_length < 5 || p_length > 51 || u_length > 51)
		return WRONG_CREDENTIALS_SIZE;

	// copio il nome utente e la password nel buffer e la grandezza del buffer nel header
	memcpy(&(msg_buf_[0]), user, u_length);
	memcpy(&(msg_buf_[u_length]), pwd, p_length);
	header_->buff_size = u_length + p_length;

	return NO_ERROR;
}

enum ERROR login(char* _params, uint8_t* msg_buf_, struct client_msg_header* header_){
	header_->buff_size = 0;
	int u_length, p_length;
	char *user, *pwd;

	// leggo i parametri
	user = _params;
	pwd = &_params[strlen(_params) + 1];

	if(*user == '\0' || *pwd == '\0'){
		return SYNTAX_ERROR;
	}
	u_length = strlen(user) + 1;
	p_length = strlen(pwd) + 1;

	// copio il nome utente e la password nel buffer e la grandezza del buffer nel header
	memcpy(&(msg_buf_[0]), user, u_length);
	memcpy(&(msg_buf_[u_length]), pwd, p_length);
	header_->buff_size = u_length + p_length;

	return NO_ERROR;
}

enum ERROR invia_giocata(char *_params, uint8_t *msg_buf_, struct client_msg_header *header_){
	header_->buff_size = 0;
	enum RUOTA r;
	// ruote_giocate e' una stringa che rappresenta le ruote giocate
	// Essendo enum, RUOTA e' un intero che va da 0 a N_RUOTE
	// ruote_giocate[R] == 'X' se la ruota R viene giocata, altrimenti '-'
	// ruote_giocate ha lunghezza N_RUOTE+1 (per il '\0')
	char ruote_giocate[N_RUOTE+1] = "-----------";

	if(strcmp(_params, "-r") != 0)
		return SYNTAX_ERROR;

	// scorro _params, leggendo le varie ruote.
	// mi fermo quando arrivo a "-n" o alla fine della stringa
	_params = &_params[strlen(_params) + 1]; //salto la stringa "-r"
	for(; strcmp(_params, "-n") != 0; _params = &_params[strlen(_params) + 1]){
		// se e' finita la stringa, errore
		if(_params[0] == '\0')
			return SYNTAX_ERROR;

		r = str2ruota(_params);
		if(r == NO_RUOTA)
			return SYNTAX_ERROR;

		if(r == TUTTE){
			strcpy(ruote_giocate, "XXXXXXXXXXX");
		}else{
			ruote_giocate[r] = 'X'; // in quanto enum, RUOTA e' un intero che va da 0 a N_RUOTE
		}
	}

	memcpy(&(msg_buf_[0]), ruote_giocate, (N_RUOTE+1) * sizeof(char));
	header_->buff_size += N_RUOTE+1;

	_params = &_params[strlen(_params) + 1]; // salto la sotto stringa già controllata "-n"

	// scorro _params, leggendo da 1 a 10 interi
	// mi fermo quando arrivo a "-i" o alla fine della stringa
	int num_counter = 0; // quanti numeri ho gia letto
	_params = &_params[strlen(_params) + 1]; //salto la stringa "-n"
	for(; strcmp(_params, "-i") != 0; _params = &_params[strlen(_params) + 1]){
		// se e' finita la stringa, oppure ho gia' letto 10 numeri, errore
		if(_params[0] == '\0' || num_counter >= 10)
			return SYNTAX_ERROR;

		int num;
		if (sscanf(_params, "%d", &num) == 0)
		 	return SYNTAX_ERROR;

		if(num > 90 || num <= 0)
				return SYNTAX_ERROR;

		// num < 90, puo' dunque essere inviato come un byte
		msg_buf_[header_->buff_size] = (uint8_t) num;
		header_->buff_size ++;
		num_counter ++;
	}

	// 0, essendo un numero invalido su cui scommettere, e' il terminatore di sequenza di numeri
	msg_buf_[header_->buff_size] = (uint8_t) 0;
	header_->buff_size ++;

	// scorro _params, leggendo gli importi (max tanti quanti i numeri e max 5)
	// mi fermo quando arrivo alla fine della stringa
	int importi_counter = 0; // quanti importi ho gia letto
	_params = &_params[strlen(_params) + 1]; // salto la sottostringa "-i"
	for(; _params[0] != '\0'; _params = &_params[strlen(_params) + 1]){
		int importo_intero = -1;
		int importo_decimale = -1;
		int numeri_letti = sscanf(_params, "%d.%d", &importo_intero, &importo_decimale);

		if (numeri_letti==0 || importo_intero<0)
				return SYNTAX_ERROR;

		// l'importo viene salvato come numero di centesimi (dunque intero)
		int importo;
		if (numeri_letti == 1) importo = importo_intero;
		if (numeri_letti == 2){
			// importo_decimale deve essere tra 0 e 99
			if (importo_decimale < 0 || importo_decimale > 99) return SYNTAX_ERROR;
			importo = importo_intero*100 + importo_decimale;
		}

		memcpy(&msg_buf_[header_->buff_size], &importo, sizeof(int));
		header_->buff_size += sizeof(int);
		importi_counter ++;
	}

	// limite sul numero di importi che ha senso siano presenti
	if (importi_counter >= num_counter || importi_counter >= 5)
		return SYNTAX_ERROR;

	return NO_ERROR;
}

enum ERROR vedi_giocata(char *_params, uint8_t *msg_buf_, struct client_msg_header *header_){
	header_->buff_size = 0;
	msg_buf_[header_->buff_size++] = (uint8_t) _params[0];
	return NO_ERROR;
}

// msg_buf: numero di estrazioni da vedere (1 byte, max 255), e la ruota (1 byte)
enum ERROR vedi_estrazione(char *_params, uint8_t *msg_buf_, struct client_msg_header *header_){
	header_->buff_size = 0;
	int n_estrazioni;
	if (sscanf(_params, "%d", &n_estrazioni) == 0)
		return SYNTAX_ERROR;
	if (n_estrazioni < 0 || n_estrazioni > 255)
		return SYNTAX_ERROR;

	_params = &_params[strlen(_params) + 1]; // vado avanti a leggere _params
	enum RUOTA r;
	if(_params[0] == '\0') // si ricorda che _params finisce con doppio \0
		r = TUTTE;
	else{
		r = str2ruota(_params);
		if (r == NO_RUOTA) return SYNTAX_ERROR;
	}
	msg_buf_[header_->buff_size++] = (uint8_t) r;
	return NO_ERROR;
}



//	************************************************** MAIN *****************************************************************
int main (int argc, char *argv[]) {
	int sid, port, i;
	char *ip_address, *str;
	struct sockaddr_in sv_addr;

	signal(SIGPIPE, disconnection_handler);

	struct client_msg_header header_to_server; // variabile che contiene l'header che il client invia al server
	struct server_msg_header header_from_server; // variabile che contiene l'header che il client riceve dal server
	uint8_t *buf = malloc(MAX_DATA_SIZE * sizeof(uint8_t)); //buffer su cui scrivere/leggere data per/da il server

	enum ERROR err;

	// il session id prima che venga configurato da login o signup è 0000000000
	strcpy(header_to_server.session_id, "0000000000");
	if(argc != 3){
		printf("ERRORE: specificare [IP] e [porta]");
		return 0;
	}
	//	estraggo i parametri di inizio
	ip_address = argv[1];
	printf("IP %s\n", ip_address);
	port = atoi(argv[2]);
	printf("PORT %d\n", port);

	// Creazione socket
	sid = socket(AF_INET, SOCK_STREAM, 0);
	// Creazione indirizzo del server
	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(port);
	inet_pton(AF_INET, ip_address, &sv_addr.sin_addr);

	i = 0;
	while(i < 10 && connect(sid, (struct sockaddr*)&sv_addr, sizeof(sv_addr)) != 0){
		sleep(1);
		i++;
	}

	if(i == 10){
		printf("impossibile connettersi al server: %s\n", strerror(errno));
		return 0;
	}

	show_help(NO_COMMAND);
	while(header_to_server.command_type != ESCI && err != BANNED && not_reachable == 0){

		printf(">");
		i = 0;
		/* estraggo la line di comando */
		size_t max_input = 1000;
		char input[1000];
		int input_size = getline((char**)&input,&max_input,stdin);
		input[input_size - 1] = '\0'; // sostiuisco il \n finale con \0
		printf("%s", input);
		err = NO_ERROR;
		// sostituisco gli spazi bianchi con \0, dividendo input in sottostringhe
		for(i = 0; i < input_size; i++){
			if(input[i] != ' ')
				continue;

			input[i] = '\0';
			if (input[i+1] == ' '){
				printf("Bad Syntax: troppi spazi\n");
				err = GENERIC_ERROR;
				break;
			}
		}

		if(err != NO_ERROR) continue; // riprova a leggere l'input

		// estraggo il comando come prima sottostringa
		header_to_server.command_type = str2command(str);
		str = &str[strlen(str) + 1];

		if(header_to_server.command_type == NO_COMMAND){
			printf("Comando non riconosciuto\n");
			continue; //riprova a leggere l'input
		}

		err = NO_ERROR;

		header_to_server.buff_size = 0;
		/*
			per ogni istruzione definisco un parser per tale istruzione nella forma
				- ERROR nomecomando(char *str, int8_t *buffer, protocol_header_client *header);
				- char* str : equivale alla stringa scritta in input senza la sottostringa del comando (EX: input = "!login\0gino\0pino" str = "gino\0pino)
				- uint8_t* buffer: buffer che verra inviato al server
				- protocol_header_client *header : header del protocollo in cui la funzione dovrà settare il parametro data_size che definisce la grandezza del buffer
				- ritorna un SYNTAX_ERROR se il comando non è ben formattato, NO_SEND se non è necessario un invio al server per eseguire la funzione, NO_ERROR altrimenti
		*/
		switch(header_to_server.command_type){
			case HELP:
				err = help(str, buf, &header_to_server);
				continue; //non serve alcuna interazione con il server
				break;
			case LOGIN:
				err = login(str, buf, &header_to_server);
				break;
			case SIGNUP:
				err = signup(str, buf, &header_to_server);
				break;
			case INVIA_GIOCATA:
				err = invia_giocata(str, buf, &header_to_server);
				break;
			case VEDI_GIOCATA:
				err = vedi_giocata(str, buf, &header_to_server);
				break;
			case VEDI_ESTRAZIONE:
				err = vedi_estrazione(str, buf, &header_to_server);
				break;
			case VEDI_VINCITE:
				err = vedi_vincite(str, buf, &header_to_server);
				break;
			case ESCI:
				err = esci(str, buf, &header_to_server);
				break;
		}

		if(err != NO_ERROR){
			show_error(err);
			continue; //riprova a leggere l'input
		}
		// invio prima l'header e buffer
		send_to_server(sid, &header_to_server, buf, &err);
		if(err != NO_ERROR){
			show_error(err);
			continue; //riprova a leggere l'input
		}
		// ricevo header a buffer
		recv_from_server(sid, &header_from_server, buf, &err);
		if(err != NO_ERROR){
			show_error(err);
			continue; //riprova a leggere l'input
		}
		if(header_from_server.error_type != NO_ERROR){
			show_error((enum ERROR)header_from_server.error_type);
			continue;
		}

		// mostro a video la risposta del server
		// oppure salvo la session_id inviata
		switch(header_to_server.command_type){
			case INVIA_GIOCATA:
				printf("Giocata inviata con successo\n");
				break;
			case LOGIN:
				strcpy(header_to_server.session_id, (char*)buf);
				break;
			case SIGNUP:
				strcpy(header_to_server.session_id, (char*)buf);
				printf("Iscrizione avvenuta con successo\n");
				break;
			case VEDI_GIOCATA:
				vedi_giocata_ret(buf, header_from_server);
				break;
			case VEDI_ESTRAZIONE:
				vedi_estrazione_ret(buf, header_from_server);
				break;
			case VEDI_VINCITE:
				vedi_vincite_ret(buf, header_from_server);
				break;
			default:
				break;
		}

	}
	close(sid);
	if(err == NO_ERROR)
		printf("Disconnessione avvenuta con successo\n");

	free(buf);
	return 0;
}
