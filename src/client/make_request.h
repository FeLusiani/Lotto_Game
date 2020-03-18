#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../shared/type.h"
#include "../shared/utils.h"
#include "help.h"

/*
Il client manda al server messaggi testuali (richieste) nel seguente formato:

"
------------------------------------------------- <HEADER>
CLIENT REQUEST\n
SESSION_ID: [SESSION ID del messaggio]\n
COMMAND: [comando, in forma di numero (enum)]\n
------------------------------------------------- <BODY>
[CAMPO]: [DATI]\n
[CAMPO]: [DATI]\n
...
-------------------------------------------------
"

*/

// la funzione make_request gestisce il comando passato e la sua stringa di input
// e scrive in msg_buf il corpo (non l'header) della richiesta da mandare al server
// restituisce NO_ERROR se non ci sono stati errori nella generazione della richiesta
// si divide in sottofunzioni, una per ogni comando
enum ERROR make_request(enum COMMAND command, char* str, char* msg_buf);
