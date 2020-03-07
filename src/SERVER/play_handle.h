typedef struct gamble_data gamble_data;

#ifndef PLAY_HANDLE_H
#define PLAY_HANDLE_H

#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include "user_handle.h"
#include "../SHARED/type.h"

 /*
 	struttura che contiene le giocate
 */
struct gamble_data{
	long t; // indica a quale estrazione appartiene
	uint16_t ruote; // indica le ruote giocabili (setting with bit-wise operation)
	short numbers[10], correct[N_RUOTE]; // numeri giocati e quantita di numeri corretti [ridondante ma abbatte la complessita]
	double values[5]; // scommesse effettuate (ambo ....)
	gamble_data *next; // per la gestione con lista
	user *u; // utente a cui appartiene la giocata
};

/*
	struttura che contiene le giocate attive, utilizzo una lista
	per avere un inserzione e una delezione costante
*/
typedef struct{
	gamble_data *head;
	pthread_mutex_t lock;
} gamble_list;

/*
	struttura per contenere le estrazioni passate
*/
typedef struct extraction extraction;
struct extraction{
  int numbers[N_RUOTE][5];
  extraction* next;
};

typedef struct{
  extraction *head;
  long last;
	pthread_mutex_t lock;
} exit_number;

exit_number* init_exit_number();

extraction* extract(exit_number *_ext);

gamble_list* init_gamble_list(users_signed *_u);

void insert_gamble(gamble_list *_g_list, gamble_data *_g_data, exit_number *_ext);

void evaluate(gamble_list *_g_list, extraction *_ext);

#endif
