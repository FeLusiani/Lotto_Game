#include "user_handle.h"

/*
	funzione di hashing trasforma una stringa e un valore in una intero
	tale funzione è utilizzata per l hash table e per l hashing della password
	non necessario data la mancanza di crittografia del canale e dell'assenza di salvataggio
	su un file registro
*/
uint32_t HASH(const char *_val, const uint32_t _seed){
	int i = 0, N = strlen(_val) + 1;
	uint32_t ret_ = 0, new_seed = rand() % (1 << 30);
  	srand(_seed);
	for(i = 0; i < 32; i++){
		srand((_val[i % N] + 12) * (rand() % 1000));
  		if((rand() % 2) == 0)
  			ret_ += (1 << i);
	}
	srand(new_seed);
	return ret_;
}

void password_hashing(char hash_[16], uint32_t _seed, char *_psw){
	int i;
	for(i = 0; i < 16; i++){
		hash_[i] = (HASH(_psw, _seed + i) % 256);
	}
	hash_[15] = '\0';

}
void insert_into(users_signed *_u, char *_username, char _hash[16], uint32_t _seed){
	user* u;
	int h = HASH(_username, 0) % _u->size; // calcolo l'ash del username per ottenre l indice del hashtabl

	u = malloc(sizeof(user));
	u->username = malloc(sizeof(char) * (strlen(_username) + 1));
	strcpy(u->username, _username);
	u->seed = _seed;
	strcpy(u->hash, _hash);
	u->list = NULL;
	pthread_mutex_init(&u->lock,  NULL);
	pthread_mutex_init(&u->lock_list,  NULL);

	/*
		prendo il lock per inserire un utente tale procedura potrebbe essere ottibizzata se si crea un lock per
		ogni lista dell'hashtable a discapito della memoria
	*/
	pthread_mutex_lock(&_u->lock);
	u->next = _u->array[h];
	_u->array[h] = u;
	pthread_mutex_unlock(&_u->lock);

}

users_signed* init_user_struct(int _size){
	FILE *fd;
	users_signed* res_ = malloc(sizeof(users_signed));
	int N, i, j;
	uint32_t seed;
	char *buffer, username[100], hash[16];

	/* inizializzo la struttura per la gestione degli utenti */
	res_->size = _size;
	res_->array = malloc(_size * sizeof(user*));

	if(pthread_mutex_init(&res_->lock, NULL) != 0){
		free(res_->array);
		free(res_);
		return NULL;
	}

	/* apro il ifle registro relativo agli utenti e leggo tutto il file per riempire la struttura */
	fd = fopen("REGISTRO/user", "r");
	fseek (fd, 0, SEEK_END);
	N = ftell (fd); // grandezza file in byte
	fseek (fd, 0, SEEK_SET);
	buffer = malloc (N * sizeof(char));
	fread (buffer, 1, N, fd); // leggo tutto il file
	fclose(fd);

	/* per ogni record del file leggo il nome l hash della password ed il seed */
	for(i = 0; i < N - 1; i++){
		j = 0;
		/* leggo il nome fino al primo spazio bianco */
		while(buffer[i] != ' '){
			username[j++] = buffer[i++];
		}
		username[j] = '\0';
		i++;

		/* leggo i successivi 15 caratteri per l'hash */
		for(j = 0; j < 15; j++)
			hash[j] = buffer[i++];
		hash[15] = '\0';
		i++;

		/* leggo il seed come intero */
		seed = get_int_from_string(&buffer[i], &j);
		i += j + 1;

		/* inserisoc i dati trovati nella struttura utenti */
		insert_into(res_, username, hash, seed);
	}

	free(buffer);
	return res_;
}

int insert_user(users_signed *_u, char *_username, char *_password){
	uint32_t seed = rand() % (1 << 31);
	char hash[16];
	FILE *fd;
	if(search_for_user(_u, _username) != NULL){
		return 0;
	}
	password_hashing(hash, seed, _password);

	/* inserisco il record nel file registro */
	fd = fopen("REGISTRO/user", "a");
	fprintf(fd, "%s %s %d\n", _username, hash, seed);
	fclose(fd);

	insert_into(_u, _username, hash, seed);


	return 1;
}

user* search_for_user(users_signed *_u, char *_username){
	int h = HASH(_username, 0) % _u->size; // calclol l hash index
	user *temp;

	/*
		prendo il lock per non avere problemi di incosistenza
		non per froza necessario ma utile
	*/
	pthread_mutex_lock(&_u->lock);
	for(temp = _u->array[h]; temp != NULL; temp = temp->next){
		if(strcmp(temp->username, _username) == 0){
			pthread_mutex_unlock(&_u->lock);
			return temp;
		}

	}
	pthread_mutex_unlock(&_u->lock);

	return NULL;
}

user* correct_login(users_signed *_u, char *_username, char *_password){
	user *temp = search_for_user(_u, _username);
	char hash[16];
	if(temp){
		password_hashing(hash, temp->seed, _password);
		/* verifico che l'hash della password sia uguale */
		if(strcmp(temp->hash, hash) == 0)
			return temp;
	}
	return NULL;
}

ip_banned_struct* init_ip_banned_struct(int _size){

	ip_banned_struct *i_ = malloc(sizeof(ip_banned_struct)); // inizializzo la struttura
	i_->size = _size;
	i_->hashtable = malloc(sizeof(ip_banned*) * _size); // inizializzo l'hashtable
	i_->list = NULL;
	pthread_mutex_init(&i_->lock, NULL);
	return i_;
}

int isBanned(ip_banned_struct* _s, uint32_t _ip){
	int h = _ip % _s->size; // calcolo l indirizzo di hash
	ip_banned *ip;
	pthread_mutex_lock(&_s->lock); // lock sulla risorsa
	ip = _s->hashtable[h];
	while(ip){ // cerco per tutta la lista di concatenamento ip di input
		if(ip->ip == _ip)
			break;
		ip = ip->next_hash;
	}

	/*
		se l'ip risulta bannato allora controllo se il tempo di ban è scaduto oppure
		ancora attivo
	*/
	if(ip && ip->tv + TIME_BANNED < time(NULL)){
		if(ip->prev_list == NULL && ip->next_list == NULL){ // un solo elemento nella lista
			_s->list = NULL;
		}else if(ip->prev_list == NULL){ // primo elemento nella lista
			_s->list = ip->next_list;
			_s->list->prev_list = NULL;
		}else if(ip->next_list == NULL){ // ultimo elemento nella lista
			ip->prev_list->next_list = NULL;
		}else{ // elemento intermedio
			ip->prev_list->next_list = ip->next_list;
			ip->next_list->prev_list = ip->prev_list;
		}

		if(ip->prev_hash == NULL && ip->next_hash == NULL){ // un solo elemento nella lista hash
			_s->hashtable[ip->ip % _s->size] = NULL;
		}else if(ip->prev_hash == NULL){ // primo elemento nella lista hash
			_s->hashtable[ip->ip % _s->size] = ip->next_hash;
			ip->next_hash->prev_hash = NULL;
		}else if(ip->next_hash == NULL){ // ultimo elemento nella lista hash
			ip->prev_hash->next_hash = NULL;
		}else{ // elemento centrale
			ip->prev_hash->next_hash = ip->next_hash;
			ip->next_hash->prev_hash = ip->prev_hash;
		}

		free(ip); // elimino
		ip = NULL;

	}

	pthread_mutex_unlock(&_s->lock); // unlock sulla risorsa
	return !(ip == NULL);
}

void insert_ip(ip_banned_struct* _s, uint32_t _ip){

	int h = _ip % _s->size;
	ip_banned *ip = malloc(sizeof(ip_banned));
	ip->ip = _ip;
	ip->tv = time(NULL);
	ip->prev_hash = NULL;
	ip->prev_list = NULL;

	pthread_mutex_lock(&_s->lock); // lock sulla risorsa

	/* inserisco l'oggetto nella hashtable (doppio link)*/
	if(_s->hashtable[h] != NULL)
		_s->hashtable[h]->prev_hash = ip;
	ip->next_hash = _s->hashtable[h];
	_s->hashtable[h] = ip;

	 /* inserisco l'oggetto nella lista di tutti gli oggetti inseriti (doppio link) */
	if(_s->list != NULL)
		_s->list->prev_list = ip;
	ip->next_list = _s->list;
	_s->list = ip;

	pthread_mutex_unlock(&_s->lock); // unlock sulla risorsa

	printf("nuovo ip bannato \n totale ip bannati:");
	for(ip = _s->list; ip; ip = ip->next_list)
		printf("%d.%d.%d.%d, ", ((ip->ip >> 0) & 255), ((ip->ip >> 8) & 255), ((ip->ip >> 16) & 255), ((ip->ip >> 24) & 255));
	printf("\n");
	fflush(stdout);
};
