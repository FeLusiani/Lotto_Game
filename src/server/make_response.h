#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <dirent.h>

#include "../shared/type.h"
#include "../shared/utils.h"
#include "server_settings.h"

typedef struct thread_slot thread_slot;

// struttura contenente i dati relativi ad un thread che serve un client
struct thread_slot{
	pthread_t tid; // id del thread
	int sid; // id del socket relativo
	char* req_buf; // buffer for request message
	char* res_buf; // buffer for response message
	char session_id[11]; // id di sessione
	char user[50]; // nome dell'user connesso
	struct in_addr ip;
	int index, n_try; // ip del client (ridondante), index indice del parametro nell'array
	int exit; // quando posto 1, causa la terminazione del thread
};

enum ERROR make_response(thread_slot* _thread_data, enum COMMAND command, char* msg_ptr, char*res_ptr);
