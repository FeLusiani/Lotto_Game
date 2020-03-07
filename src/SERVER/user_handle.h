typedef struct user user;
typedef struct users_signed users_signed;

#ifndef USER_HANDLE_H
#define USER_HANDLE_H

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>

#include "play_handle.h"
#include "../SHARED/utils.h"

// tempo prima che un utente venga rimosso dalla lista degli ip bannati
#define TIME_BANNED 30 * 60
/*
	gli user sono salvati in una hashtable di dimensione fissa essendo veloce il loro reperimento
	con lista per elementi in collisiome
*/
struct user{
	char *username;
	char hash[16];
	uint32_t seed;
	pthread_mutex_t lock, lock_list;
	user* next;
	gamble_data *list; // lista delle giocate non attive
};

/*
	gli ip sono salvati in una altra hash table con una doppia lista ridondante
	in modo tale che siano facilmente eseguibili in sequenza ma l'inserimento e
	la delezione siano in complessita O(1)
*/
typedef struct ip_banned ip_banned;

struct ip_banned{
	uint32_t ip; // ip bannato
	time_t tv; // time stamp
	ip_banned *next_hash, *prev_hash, *next_list, *prev_list;
};

typedef struct{
	ip_banned **hashtable;
	ip_banned* list;
	int size; // quantità di elementi vs numero massimo di elementi
	pthread_mutex_t lock; // lock per l'accesso in mutua esclusione
}ip_banned_struct;

/*
	hashtable degli utenti
*/
struct users_signed{
	user **array;
	int size;
	pthread_mutex_t lock;
};

users_signed* init_user_struct(int _size); // crea la struttura users_signed

int insert_user(users_signed *_u, char *_username, char *_password); // inserisce un utente

user* search_for_user(users_signed *_u, char *_username); // cerca un utente

user* correct_login(users_signed *_u, char *_username, char *_password); // verifica se il login è corretto

ip_banned_struct* init_ip_banned_struct(int _size); // crea la struttura che contiene gli ip bannati

int isBanned(ip_banned_struct* _i, uint32_t _ip); // controlla se un determinato ip è bannato e se il tempo di tale ip è scaduto

void insert_ip(ip_banned_struct *_s, uint32_t _ip); // inserisce per un tempo _end un ip


#endif
