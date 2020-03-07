#include <string.h>
#include <stdio.h>
#include "../SHARED/type.h"

int get_int_from_string(char* _str, int *num_);

int isRuota(enum RUOTA r);

enum COMMAND str2command(char *_c);

enum RUOTA str2ruota(char *_c);

const char* ruota2str(enum RUOTA);

void show_double(double val);

int strlenbelow(char* str, int max, int *flag);

int disposizioni(int N, int k); // calcolo delle disposizioni
