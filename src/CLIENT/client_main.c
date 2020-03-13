#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "../SHARED/type.h"
#include "../SHARED/utils.h"
#include "../SHARED/networking.h"
#include "message.h"
#include "make_request.h"

#define min(X, Y)  ((X) < (Y) ? (X) : (Y))

int not_reachable = 0;
void disconnection_handler(int signal){
	printf("Error: not connected to server\n");
	not_reachable = 1;
}


enum ERROR display_giocata(char* _msg){
	printf("%s", _msg);
	return NO_ERROR;
}

enum ERROR display_estrazione(char* _msg){
	printf("%s", _msg);
	return NO_ERROR;
}

enum ERROR display_vincite(char* _msg){
	printf("%s", _msg);
	return NO_ERROR;
}


//	************************************************** MAIN *****************************************************************
int main (int argc, char *argv[]) {
	int sid, port;
	char *ip_address;
	struct sockaddr_in sv_addr;
	signal(SIGPIPE, disconnection_handler);

	if(argc != 3){
		printf("ERRORE: specificare [IP] e [porta]\n");
		return 0;
	}
	// estraggo i parametri
	ip_address = argv[1];
	port = atoi(argv[2]);

	char* msg_buf = (char*)malloc(MAX_MSG_LENGTH); //buffer su cui scrivere/leggere data per/da il server
	char session_id[SESS_ID_SIZE] = "0000000000";
	enum ERROR err = NO_ERROR;
	enum COMMAND command = NO_COMMAND;


	// Creazione socket
	sid = socket(AF_INET, SOCK_STREAM, 0);
	// Creazione indirizzo del server
	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(port);
	inet_pton(AF_INET, ip_address, &sv_addr.sin_addr);

	printf("Connecting to server...\n");
	int i = 0;
	while(i < 10 && connect(sid, (struct sockaddr*)&sv_addr, sizeof(sv_addr)) != 0){
		sleep(1);
		i++;
	}

	if(i == 10){
		printf("impossibile connettersi al server: %s\n", strerror(errno));
		return 0;
	}

	show_help(NO_COMMAND);
	while(command != ESCI && err != BANNED && err != DISCONNECTED && not_reachable == 0){
		// mostra eventuale errore da ciclo precedente
		if (err != NO_ERROR)
			show_error(err);
		err = NO_ERROR;
		// preparazione messaggio
		strcpy(msg_buf, "");
		sprintf(&msg_buf[strlen(msg_buf)], "CLIENT REQUEST\nSESSION_ID: %s\n", session_id);

		printf(">");
		// leggo linea di comando
		int max_input = 1000;
		char input[1000];
		if(fgets(input,max_input,stdin) == NULL)
			continue;
		int input_size = strlen(input);
		input[input_size - 1] = '\0'; // sostiuisco il \n finale con \0
		// sostituisco gli spazi bianchi con \0, dividendo input in sottostringhe
		for(i = 0; i < input_size; i++){
			if(input[i] != ' ')
				continue;

			input[i] = '\0';
			if (input[i+1] == ' '){
				printf("Bad Syntax: troppi spazi\n");
				err = SYNTAX_ERROR;
				break;
			}
		}

		if(err != NO_ERROR) continue; // riprova a leggere l'input

		// estraggo il comando come prima sottostringa
		char* str = input;
		command = str2command(str);
		str = &str[strlen(input) + 1];
		sprintf(&msg_buf[strlen(msg_buf)], "COMMAND: %d\n", (int)command);

		err = make_request(command, str, msg_buf);

		if (command == HELP) continue; // help non necessita di inviare alcuna richiesta
		if(err != NO_ERROR) continue; //se errore, riprova a leggere l'input
		// invio la richiesta
		err = send_msg(sid, msg_buf);
		if(err != NO_ERROR) continue; //riprova a leggere l'input
		// ricevo la risposta
		err = get_msg(sid, msg_buf);
		if(err != NO_ERROR) continue; //riprova a leggere l'input

		printf("\n%s\n", msg_buf); /////////////////////////////////////////////////////////////////
		fflush(stdout);

		char* msg_ptr = msg_buf; //pointer per parsare la risposta
		msg_ptr = next_line(msg_ptr); // salto la prima linea "SERVER RESPONSE"
		//leggo eventuali errori
		int errore_letto  = sscanf(msg_ptr, "ERROR: %d\n", (int*)&err);
		if(errore_letto == 1) continue; //riprova a leggere l'input

		// mostro a video la risposta del server
		// oppure salvo la session_id inviata
		switch(command){
			case INVIA_GIOCATA:
				printf("Giocata inviata con successo\n");
				break;
			case LOGIN:
				sscanf(msg_ptr, "SESSION_ID: %s\n", session_id);
				printf("Login avvenuto con successo\n");

				printf("SESSION ID: %s", session_id);
				break;
			case SIGNUP:
				printf("Iscrizione avvenuta con successo\n");
				break;
			case VEDI_GIOCATA:
				err = display_giocata(msg_ptr);
				break;
			case VEDI_ESTRAZIONE:
				err = display_estrazione(msg_ptr);
				break;
			case VEDI_VINCITE:
				err = display_vincite(msg_ptr);
				break;
			default:
				break;
		}

	}

	close(sid);
	if(err == NO_ERROR)
		printf("Disconnessione avvenuta con successo\n");
	else{
		printf("Disconnessione\n");
		show_error(err);
	}

	free(msg_buf);
	return 0;
}
