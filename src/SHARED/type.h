#ifndef TYPES
#define TYPES


#define MAX_MSG_LENGTH 1000000
#define N_RUOTE 11
#define SESS_ID_SIZE 11

#include <stdint.h>

/* enum per i comandi inviabili dal client */
enum COMMAND{HELP, SIGNUP, LOGIN, INVIA_GIOCATA, VEDI_GIOCATA, VEDI_ESTRAZIONE, VEDI_VINCITE, ESCI, NO_COMMAND};

/* ruote possibili */
enum RUOTA{BARI, CAGLIARI, FIRENZE, GENOVA, MILANO, NAPOLI, PALERMO, ROMA, TORIO, VENEZIA, NAZIONALE, TUTTE, NO_RUOTA};

/* errori possibili generabili dal client o dal server per una descrizione piu dettagliata andare in messaggi.c */
enum ERROR{SERVER_ERROR, BAD_REQUEST, NOT_LOGGED_IN, NO_COMMAND_FOUND, WRONG_CREDENTIALS_SIZE, NO_ERROR, USER_ALREADY_TAKEN, WRONG_CREDENTIALS, WRONG_SESSID, ALREADY_LOGGED, THREAD_LOGGED_IN, GENERIC_ERROR, SYNTAX_ERROR, BANNED, DISCONNECTED, MAX_SIZE_REACH};


#endif
