#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "type.h"

// invia il messaggio testuale _msg tramite il socket _sid
enum ERROR send_msg(int _sid, char* _msg);
// usato dal server per inviare un messaggio di errore
enum ERROR send_error(int _sid, enum ERROR e);
// riceve un messaggio testuale da _sid e le scrive in msg_
enum ERROR get_msg(int _sid, char* msg_);
