#ifndef TYPES
#define TYPES

/* numero totale di comandi inviabili (NO_COMMAND non conta come comando) */
#define N_COMMANDS 8

/* numero totale di errori inviabili (NO_ERROR non conta come errore) */
#define N_ERRORS 20

/* numero totale di ruote giocabili (TUTTE e NO_RUOTA non contano come ruote) */
#define N_RUOTE 11

#define MAX_DATA_SIZE 1000000

#include <stdint.h>

/* enum per i comandi inviabili dal client */
enum COMMAND{HELP, SIGNUP, LOGIN, INVIA_GIOCATA, VEDI_GIOCATA, VEDI_ESTRAZIONE, VEDI_VINCITE, ESCI, NO_COMMAND};

/* ruote possibili */
enum RUOTA{BARI, CAGLIARI, FIRENZE, GENOVA, MILANO, NAPOLI, PALERMO, ROMA, TORIO, VENEZIA, NAZIONALE, TUTTE, NO_RUOTA};

/* errori possibili generabili dal client o dal server per una descrizione piu dettagliata andare in messaggi.c */
enum ERROR{NOT_SIGNED, NO_COMMAND_FOUND, CREDENTIAL_WRONG_SIZE, NO_ERROR, USER_ALREADY_TAKEN, CREDENTIAL_NOT_CORRECT, WRONG_SESSID, ALREADY_LOGGED, THREAD_LOGGED_IN, GENERIC_ERROR, BAD_FORMAT, SYNTAX_ERROR, BANNED, NO_SEND, DISCONNECTED, MAX_SIZE_REACH};

/* struttura header per i messaggi inviati dal client */
struct protocol_header_client{
	uint32_t data_size; // lunghezza del messaggio incapsulato
	uint8_t command_type; // tipo di comando inviato dal client (enum ruota)
	char session_id[11];  // id di sessione ad inizio sessione deve essere impostato a 10 '0'
};

/* struttura header per i messaggi inviati dal server*/
struct protocol_header_server{
	uint32_t data_size; // lunghezza del messaggio incapsulato
	uint8_t error_type; // tipo di errore (se non ci sono errori NO_ERROR)
};

#endif
