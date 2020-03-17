#ifndef UTILS
#define UTILS

#include <string.h>
#include <stdio.h>
#include "../SHARED/type.h"

void show_error(enum ERROR e);

char* next_line(char* str);

int leggi_importo(const char* str);

enum COMMAND str2command(char *_c);

enum RUOTA str2ruota(char *_c);

const char* ruota2str(enum RUOTA);

void show_double(double val);

int strlenbelow(char* str, int max, int *flag);

int disposizioni(int N, int k); // calcolo delle disposizioni

#endif
