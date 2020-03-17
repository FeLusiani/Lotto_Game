#include "make_request.h"
#define min(X, Y)  ((X) < (Y) ? (X) : (Y))
// le seguenti funzioni sono utilizzate per eseguire i comandi del client
// ogni funzione si occupa di scrivere il messaggio da scrivere al server (header e buffer)
// Se non ci sono errori, restituisce NO_ERROR

enum ERROR help(char *_params, char* msg_){
	if(_params[0] == '\0'){
		show_help(NO_COMMAND);
		return NO_ERROR;
	}
	enum COMMAND c = str2command(_params);
	if(c == NO_COMMAND)
		printf("Non esiste alcun comando con questo nome\nUsa !help per vedere la lista dei comandi\n");
	else
		show_help(c);

	return NO_ERROR;
}

enum ERROR signup(char *_params, char* msg_){
	int u_length, p_length;
	char *user, *pwd;

	// leggo i parametri
	user = _params;
	pwd = &_params[strlen(_params) + 1];

	if(*user == '\0' || *pwd == '\0') return SYNTAX_ERROR;

    for (int i=0; user[i] != '\0'; i++){
        if (!isalnum(user[i])){
            printf("Username deve avere valori alfanumerici");
            return SYNTAX_ERROR;
        }
    }
	u_length = strlen(user) + 1;
	p_length = strlen(pwd) + 1;

	// credentials size tra 4 e 50 char
	if(p_length < 5 || u_length < 5 || p_length > 51 || u_length > 51)
		return WRONG_CREDENTIALS_SIZE;

	sprintf(&msg_[strlen(msg_)], "USER: %s\n", user);
	sprintf(&msg_[strlen(msg_)], "PWD: %s\n", pwd);

	return NO_ERROR;
}

enum ERROR login(char *_params, char* msg_){
	char *user, *pwd;

	// leggo i parametri
	user = _params;
	pwd = &_params[strlen(_params) + 1];

	if(*user == '\0' || *pwd == '\0'){
		return SYNTAX_ERROR;
	}

	sprintf(&msg_[strlen(msg_)], "USER: %s\n", user);
	sprintf(&msg_[strlen(msg_)], "PWD: %s\n", pwd);

	return NO_ERROR;
}

enum ERROR invia_giocata(char *_params, char* msg_){
	enum RUOTA r;
	// ruote_giocate e' una stringa che rappresenta le ruote giocate
	// Essendo enum, RUOTA e' un intero che va da 0 a N_RUOTE
	// ruote_giocate[R] == 'X' se la ruota R viene giocata, altrimenti '-'
	// ruote_giocate ha lunghezza N_RUOTE+1 (per il '\0')
	char ruote_giocate[N_RUOTE+1] = "-----------";

	if(strcmp(_params, "-r") != 0)
		return SYNTAX_ERROR;

	// scorro _params, leggendo le varie ruote.
	// mi fermo quando arrivo a "-n" o alla fine della stringa
	_params = &_params[strlen(_params) + 1]; //salto la stringa "-r"
	for(; strcmp(_params, "-n") != 0; _params = &_params[strlen(_params) + 1]){
		// se e' finita la stringa, errore
		if(_params[0] == '\0') return SYNTAX_ERROR;

		r = str2ruota(_params);
		if(r == NO_RUOTA){
			printf("La ruota indicata non esiste\n");
			return SYNTAX_ERROR;
		}

		if(r == TUTTE)
			strcpy(ruote_giocate, "XXXXXXXXXXX");
		else
			ruote_giocate[r] = 'X'; // in quanto enum, RUOTA e' un intero che va da 0 a N_RUOTE
	}
	sprintf(&msg_[strlen(msg_)], "RUOTE_GIOCATE: %s\n", ruote_giocate);

	// scorro _params, leggendo da 1 a 10 interi
	// mi fermo quando arrivo a "-i" o alla fine della stringa
	sprintf(&msg_[strlen(msg_)], "NUMERI_GIOCATI:"); // inizio linea dei numeri giocati
	int num_counter = 0; // quanti numeri ho gia letto
	_params = &_params[strlen(_params) + 1]; //salto la stringa "-n"
	for(; strcmp(_params, "-i") != 0; _params = &_params[strlen(_params) + 1]){
		// se e' finita la stringa, oppure ho gia' letto 10 numeri, errore
		if(_params[0] == '\0' || num_counter >= 10) return SYNTAX_ERROR;
		int num;
		if (sscanf(_params, "%d", &num) == 0) return SYNTAX_ERROR;
		if(num > 90 || num <= 0) return SYNTAX_ERROR;
		sprintf(&msg_[strlen(msg_)], " %d", num);
		num_counter ++;
	}
	sprintf(&msg_[strlen(msg_)], "\n"); // chiudo la linea dei numeri giocati

	// scorro _params, leggendo gli importi (tanti quanti min(numeri giocati, N_COMBO))
	// mi fermo quando arrivo alla fine della stringa
	sprintf(&msg_[strlen(msg_)], "IMPORTI:"); // inizio linea dei numeri giocati
	int importi_counter = 0; // quanti importi ho gia letto
	_params = &_params[strlen(_params) + 1]; // salto la sottostringa "-i"
	for(; _params[0] != '\0'; _params = &_params[strlen(_params) + 1]){
		int importo = leggi_importo(_params);
		if (importo == -1) return SYNTAX_ERROR;
		sprintf(&msg_[strlen(msg_)], " %d", importo);
		importi_counter ++;
	}

	// condizione sul numero di importi che devo aver letto
	if (importi_counter != min(num_counter,N_COMBO)){
			printf("Numero di importi indicati scorretto\n");
			return SYNTAX_ERROR;
	}
	// aggiungo importi nulli finche' importi_counter non e' a 5
	for (; importi_counter<N_COMBO; importi_counter++){
		sprintf(&msg_[strlen(msg_)], " %d", 0);
	}
	sprintf(&msg_[strlen(msg_)], "\n"); // chiudo la linea dei numeri giocati

	return NO_ERROR;
}

enum ERROR vedi_giocate(char *_params, char* msg_){
	if (_params[0] != '0' && _params[0] != '1'){
		printf("TIPO deve valere 0 o 1\n");
		return SYNTAX_ERROR;
	}
	sprintf(&msg_[strlen(msg_)], "TIPO: %c\n", _params[0]);
	return NO_ERROR;
}

enum ERROR vedi_estrazioni(char *_params, char* msg_){
	if (_params[0] == '\0') return SYNTAX_ERROR;
	int n_estrazioni;
	if (sscanf(_params, "%d", &n_estrazioni) == 0)
		return SYNTAX_ERROR;
	if (n_estrazioni < 0)
		return SYNTAX_ERROR;
	sprintf(&msg_[strlen(msg_)], "N_ESTRAZIONI: %d\n", n_estrazioni);

	_params = &_params[strlen(_params) + 1]; // vado avanti a leggere _params
	enum RUOTA r;
	if(_params[0] == '\0') // si ricorda che _params finisce con doppio \0
		r = TUTTE;
	else{
		r = str2ruota(_params);
		if (r == NO_RUOTA) return SYNTAX_ERROR;
	}
	sprintf(&msg_[strlen(msg_)], "RUOTA: %d\n", (int)r);
	return NO_ERROR;
}

enum ERROR vedi_vincite(char *_params, char* msg_){
	return NO_ERROR;
}

enum ERROR esci(char *_params, char* msg_){
	return NO_ERROR;
}







enum ERROR make_request(enum COMMAND command, char* str, char* msg_buf){
	enum ERROR e_;
	switch(command){
		case HELP:
			e_ = help(str, msg_buf);
			break;
		case LOGIN:
			e_ = login(str, msg_buf);
			break;
		case SIGNUP:
			e_ = signup(str, msg_buf);
			break;
		case INVIA_GIOCATA:
			e_ = invia_giocata(str, msg_buf);
			break;
		case VEDI_GIOCATE:
			e_ = vedi_giocate(str, msg_buf);
			break;
		case VEDI_ESTRAZIONI:
			e_ = vedi_estrazioni(str, msg_buf);
			break;
		case VEDI_VINCITE:
			e_ = vedi_vincite(str, msg_buf);
			break;
		case ESCI:
			e_ = esci(str, msg_buf);
			break;
		case NO_COMMAND:
			e_ = NO_COMMAND_FOUND; //riprova a leggere l'input
			break;
	}
	return e_;
}
