#include "networking.h"

enum ERROR send_msg(int _sid, char* _msg){
	ssize_t res;
	uint16_t msg_len = strlen(_msg)+1;
	uint16_t buf_size = htons(msg_len);

	//invio la lunghezza della stringa da ricevere
	res = send(_sid, (void*)&buf_size, sizeof(uint16_t), 0);
	if(res == -1) return DISCONNECTED;
	//invio la stringa
	res = send(_sid, (void*)_msg, msg_len, 0);
	if(res == -1) return DISCONNECTED;

	return NO_ERROR;
}

/* usato dal server per inviare un messaggio di errore nel formato
"
SERVER RESPONSE\n
ERROR: [errore in forma di numero (enum)]
"
*/
enum ERROR send_error(int _sid, enum ERROR e){
	char buffer[100];
	sprintf(buffer, "SERVER RESPONSE\nERROR: %d\n", (int)e);
	return send_msg(_sid, buffer);
}

// riceve un messaggio testuale da _sid e le scrive in msg_
enum ERROR get_msg(int _sid, char* msg_){
	ssize_t res;
	uint16_t buf_size;
	uint16_t msg_len;

	//ricevo la lunghezza della stringa da ricevere
	res = recv(_sid, (void*)&buf_size, sizeof(uint16_t), 0);
	if(res == -1 || res == 0) return DISCONNECTED;
	//ricevo la stringa
	msg_len = ntohs(buf_size);
	res = recv(_sid, (void*)msg_, msg_len, 0);
	if(res == -1 || res == 0) return DISCONNECTED;

	return NO_ERROR;
}
