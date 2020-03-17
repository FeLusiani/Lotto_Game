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


enum ERROR display_giocate(char* _msg){
	const char* format_giocata = "RUOTE_GIOCATE: %s\n"
								 "NUMERI_GIOCATI:%[^\n]\n"
								 "IMPORTI: %d %d %d %d %d\n%n";
	char ruote_giocate[N_RUOTE+1];
	char num_giocati[50];
	int importi[N_COMBO];
	int chars_read;
	int giocate_counter = 1;
	while(1){
		printf("\n");
		int res = sscanf(_msg, format_giocata, ruote_giocate, num_giocati,
						 &importi[0], &importi[1], &importi[2], &importi[3], &importi[4],
						 &chars_read);

		if (res < 7) break;

		_msg = &_msg[chars_read]; // avanzo il puntatore per parsare le giocate
		printf(" %d)", giocate_counter++);
		// stampa le ruote
		if (strcmp(ruote_giocate, "XXXXXXXXXXX") == 0)
			printf(" tutte");
		else{
			for (int i=0; i<N_RUOTE; i++){
				if (ruote_giocate[i] == '-') continue;
				enum RUOTA r = (enum RUOTA ) i;
				printf(" %s", ruota2str(r));
			}
		}
		// stampo i numeri giocati
		printf("%s", num_giocati);
		// stampo gli importi in ordine inverso
		const char* combo[N_COMBO];
		combo[0] = "estratto";
		combo[1] = "ambo";
		combo[2] = "terno";
		combo[3] = "quaterna";
		combo[4] = "cinquina";
		for (int i=N_COMBO-1; i>=0; i--){
			if (importi[i] == 0)
				continue;
			double importo_euro = (float)importi[i] / 100;
			printf(" * %.2f %s", importo_euro, combo[i]);
		}

	}
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

	char* const msg_buf = (char*)malloc(MAX_MSG_LENGTH); //buffer su cui scrivere/leggere data per/da il server
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
		printf("Impossibile connettersi al server: %s\n", strerror(errno));
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

		// printf("\n%s\n", msg_buf); /////////////////////////////////////////////////////////////////
		// fflush(stdout);

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
				break;
			case SIGNUP:
				printf("Iscrizione avvenuta con successo\n");
				break;
			case VEDI_GIOCATE:
				err = display_giocate(msg_ptr);
				break;
			case VEDI_ESTRAZIONI:
				printf("%s\n", msg_ptr);
				break;
			case VEDI_VINCITE:
				printf("%s\n", msg_ptr);
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
