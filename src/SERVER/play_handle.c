#include "play_handle.h"

void writeBytes(FILE* _fd, void* _data, size_t _size){
	int i;
	for(i = 0; i < _size; i++)
		fprintf(_fd, "%c", ((uint8_t*)_data)[i]);
}
int readBytes(uint8_t *_buffer, void *res_, size_t _size){
	int i;
	for(i = 0; i < _size; i++){
		((uint8_t*)res_)[i] = _buffer[i];
	}
	return _size;
}

exit_number* init_exit_number(){
	exit_number *res = malloc(sizeof(exit_number));
	extraction *ext;
	FILE *fd;
	int N, i;
	char *buffer;

	pthread_mutex_init(&res->lock, NULL);
	res->head = NULL;
	res->last = 0;
	/* apro il ifle registro relativo alle giocate e leggo tutto il file per riempire la struttura */
	fd = fopen("REGISTRO/extraction", "r");
	fseek (fd, 0, SEEK_END);
	N = ftell (fd); // grandezza file in byte
	fseek (fd, 0, SEEK_SET);
	buffer = malloc (N * sizeof(char));
	fread (buffer, 1, N, fd); // leggo tutto il file
	fclose(fd);

	for(i = 0; i < N;){
		ext = malloc(sizeof(extraction));
		i += readBytes((uint8_t*)&buffer[i], ext->numbers, sizeof(ext->numbers[0][0]) * N_RUOTE * 5);
		ext->next = res->head;
	 	res->head = ext;
		res->last++;
	}
	return res;

}

extraction* extract(exit_number *_ext){
	int i, j, index, temp;
	FILE *fd;
	extraction *res = malloc(sizeof(extraction));
	int numeri_possibili[90];

	/* inderisco i numeri possibili  [1, 90] */
	for(i = 0; i <  90; i++)
		numeri_possibili[i] = i + 1;

	/* la lista g_list contiene tutte le giocate (attive) inserite ma non ancora osservate */
	pthread_mutex_lock(&_ext->lock);

	printf("inizio estrazione.....\n");fflush(stdout);

	fd = fopen("REGISTRO/extraction", "a");

	/* per ogni ruota estraggo casualmente 5 numeri */
	for(i = 0; i < N_RUOTE; i++){
		printf(" - %s : ", ruota2str(i));fflush(stdout);
		for(j = 0; j < 5; j++){
			index = rand() % (90 - j); // estrazione di una posizione casuale nell'array numeri possibili

			printf("%d,", numeri_possibili[index]);fflush(stdout);
			res->numbers[i][j] = numeri_possibili[index]; // inserisco il numero fra quelli estratti
			writeBytes(fd, &numeri_possibili[index], sizeof(numeri_possibili[index]));

			/*  scambio del numero estratto con l'ultimo numero dell'array (per impedire estrazioni uguali successive)*/
			temp = numeri_possibili[89 - j];
			numeri_possibili[89 - j] = numeri_possibili[index];
			numeri_possibili[index] = temp;
		}
		printf("\n");fflush(stdout);
	}
	fclose(fd);

	res->next = _ext->head;
	_ext->head = res;
	_ext->last++;
	
	pthread_mutex_unlock(&_ext->lock);

	return res;
}

gamble_list* init_gamble_list(users_signed *_u){
	gamble_list* l = malloc(sizeof(gamble_list));
	FILE *fd;
	user *u;
	gamble_data *g;
	int N, i, j;
	char *buffer, username[100];
	l->head = NULL;
	pthread_mutex_init(&l->lock,  NULL);

	/* apro il ifle registro relativo alle giocate e leggo tutto il file per riempire la struttura */
	fd = fopen("REGISTRO/betting", "r");
	fseek (fd, 0, SEEK_END);
	N = ftell (fd); // grandezza file in byte
	fseek (fd, 0, SEEK_SET);
	buffer = malloc (N * sizeof(char));
	fread (buffer, 1, N, fd); // leggo tutto il file
	fclose(fd);

	/* inizio il riempimento della struttura che contiene le giocate effettuate */
	for(i = 0; i < N; i++){
		j = 0;
		g = malloc(sizeof(gamble_data));

		/* leggo il nome utente */
		while(buffer[i] != ' '){
			username[j++] = buffer[i++];
		}
		username[j] = '\0';
		i++;

		/* leggo a quale estrazione appartiene la giocata */
		i += readBytes((uint8_t*)&buffer[i], &g->t, sizeof(g->t));

		/* leggo le ruote su cui è stata effettuata la giocata */
		i += readBytes((uint8_t*)&buffer[i], &g->ruote, sizeof(g->ruote));

		/* leggo i numeri giocati */
		for(j = 0; j < 10; j++){
				i += readBytes((uint8_t*)&buffer[i], &g->numbers[j], sizeof(g->numbers[j]));
		}

		/* leggo le scommesse effettuate */
		for(j = 0; j < 5; j++){
				i += readBytes((uint8_t*)&buffer[i], &g->values[j], sizeof(g->values[j]));
		}

		/* leggo i numeri indovinati */
		for(j = 0; j < N_RUOTE; j++)
			g->correct[j] = 0;

		for(j = 0; j < N_RUOTE; j++){
				if((g->ruote & (1 << j)) != 0){
					i += readBytes((uint8_t*)&buffer[i], &g->correct[j], sizeof(g->correct[j]));
				}
		}

		/*
			cerco l'utente che ha effettuato le giocate ed inserisco la giocata trovata
			questa operazione effettuata per ogni utente potrebbe risultare piuttosto lenta
			e risulta difficile da migliorare a meno di non ordinare il file registro ma tale
			ordinamento potrebbe rallentare l 'inserimento delle giocate.... inoltre questa
			funzione viene chiamata solo all'inizio del programma quindi non è necessaria
			un ottima prestazione
		*/
		u = search_for_user(_u, username);
		g->next = u->list;
		u->list = g;
	}
	free(buffer);
	return l;
}

void insert_gamble(gamble_list *_g_list, gamble_data *_g_data, exit_number *_ext){
	pthread_mutex_lock(&_ext->lock);
	_g_data->t = _ext->last;
	pthread_mutex_unlock(&_ext->lock);

	pthread_mutex_lock(&_g_list->lock);
	_g_data->next = _g_list->head;
	_g_list->head = _g_data;
	pthread_mutex_unlock(&_g_list->lock);
}


void evaluate(gamble_list *_g_list, extraction *_ext){
	gamble_data *data, *temp_g;
	int i, j, k;
	FILE *fd;
	/*
		a questo punto inserisco gli elementi della lista dei giocati ma non controllati all'interno
		della lista per giocatore inserendo anche il risultato ottenuto
	*/

	fd = fopen("REGISTRO/betting", "a");

	data = _g_list->head;
	while(data){
		temp_g = data->next;
		/*
			 inserisco l'elemento nella lista utenti (non ho bisogno di alcun lock essendo le uniche funzioni
			 su tale lista di lettura con inconsistenza accettabile per breve tempo)
		*/
		data->next = data->u->list;
		data->u->list = data;
		for(i = 0; i < N_RUOTE; i++){
			data->correct[i] = 0;
			/*
				per ogni ruota confronto il risultato con quello ottenuto
			*/
			if((data->ruote & (1 << i)) != 0){
				for(j = 0; j < 10; j++){
					for(k = 0; k < 5; k++){
						if(data->numbers[j] == _ext->numbers[i][k]){
							data->correct[i] = (data->correct[i] == 5) ? 5 : (data->correct[i] + 1);
						}
					}
				}
			}
		}

		fprintf(fd, "%s ", data->u->username);

		writeBytes(fd, &data->t, sizeof(data->t));

		writeBytes(fd, &data->ruote, sizeof(data->ruote));

		/* scrivo i numeri giocati */
		for(i = 0; i < 10; i++){
			writeBytes(fd, &data->numbers[i], sizeof(data->numbers[i]));
		}
		/* scrivo il valore delle scommesse */
		for(i = 0; i < 5; i++){
			writeBytes(fd, &data->values[i], sizeof(data->values[i]));
		}

		/* scrivo la quantita di numeri corretti */
		for(i = 0; i < N_RUOTE; i++){
				if((data->ruote & (1 << i)) != 0){
					writeBytes(fd, &data->correct[i], sizeof(data->correct[i]));
				}
		}
		fprintf(fd, "\n");

		data = temp_g;
	}
	_g_list->head = NULL;

	fclose(fd);

}
