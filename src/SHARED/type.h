#ifndef TYPES
#define TYPES


#define MAX_DATA_SIZE 1000000
#define N_RUOTE 11

#include <stdint.h>

/* enum per i comandi inviabili dal client */
enum COMMAND{HELP, SIGNUP, LOGIN, INVIA_GIOCATA, VEDI_GIOCATA, VEDI_ESTRAZIONE, VEDI_VINCITE, ESCI, NO_COMMAND};

/* ruote possibili */
enum RUOTA{BARI, CAGLIARI, FIRENZE, GENOVA, MILANO, NAPOLI, PALERMO, ROMA, TORIO, VENEZIA, NAZIONALE, TUTTE, NO_RUOTA};

/* errori possibili generabili dal client o dal server per una descrizione piu dettagliata andare in messaggi.c */
enum ERROR{NOT_SIGNED, NO_COMMAND_FOUND, WRONG_CREDENTIALS_SIZE, NO_ERROR, USER_ALREADY_TAKEN, CREDENTIAL_NOT_CORRECT, WRONG_SESSID, ALREADY_LOGGED, THREAD_LOGGED_IN, GENERIC_ERROR, BAD_FORMAT, SYNTAX_ERROR, BANNED, DISCONNECTED, MAX_SIZE_REACH};

/* struttura header per i messaggi inviati dal client */
struct client_msg_header{
	uint32_t buff_size; // lunghezza del messaggio incapsulato
	uint8_t command_type; // tipo di comando inviato dal client
	char session_id[11];  // id di sessione, parte come 10 caratteri '0'
};

/* struttura header per i messaggi inviati dal server*/
struct server_msg_header{
	uint32_t buff_size; // lunghezza del messaggio incapsulato
	uint8_t error_type; // tipo di errore (se non ci sono errori NO_ERROR)
};

#endif
