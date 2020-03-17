#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "type.h"

enum ERROR send_msg(int _sid, char* _msg);
enum ERROR send_error(int _sid, enum ERROR e);

enum ERROR get_msg(int _sid, char* msg_);
