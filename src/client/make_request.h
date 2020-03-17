#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../shared/type.h"
#include "../shared/utils.h"
#include "help.h"

// le seguenti funzioni sono utilizzate per eseguire i comandi del client
// ogni funzione si occupa di scrivere il messaggio da scrivere al server (header e buffer)
// Se non ci sono errori, restituisce NO_ERROR
enum ERROR make_request(enum COMMAND command, char* str, char* msg_buf);
