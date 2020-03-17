#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../SHARED/type.h"
#include "../SHARED/utils.h"
#include "message.h"

// le seguenti funzioni sono utilizzate per eseguire i comandi del client
// ogni funzione si occupa di scrivere il messaggio da scrivere al server (header e buffer)
// Se non ci sono errori, restituisce NO_ERROR
enum ERROR make_request(enum COMMAND command, char* str, char* msg_buf);
