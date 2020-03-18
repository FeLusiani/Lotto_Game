#include "utils.h"

// restituisce il puntatore alla prossima linea (prima pos dopo un \n)
// Se non trova nessun \n, restituisce NULL
// la funzione cerca fino a quando non trova \0, fino a max 1000 caratteri
char* next_line(char* str){
	for(int i=0; str[i] != '\0' && i < 1000; i++){
		if (str[i] == '\n')
			return &str[i+1];
	};
	return NULL;
}

// legge una stringa contenente un importo in euro e restituisce il numero di centesimi
// restituisce -1 in caso di errore
int leggi_importo(const char* str){
	int importo_intero = -1;
	char cifra1_cent = '/';
	char cifra2_cent = '/';
	int numeri_letti = sscanf(str, "%d.%c%c", &importo_intero, &cifra1_cent, &cifra2_cent);

	if (numeri_letti==0 || importo_intero<0) return -1;

	int importo; // l'importo viene salvato come numero di centesimi (dunque intero)
	switch(numeri_letti){
		case 1:
			importo = importo_intero*100;
			break;
		case 2:
			if (cifra1_cent < '0' || cifra1_cent > '9') return -1;
			importo = importo_intero*100 + (cifra1_cent-'0')*10;
			break;
		case 3:
			if (cifra1_cent < '0' || cifra1_cent > '9') return -1;
			if (cifra2_cent < '0' || cifra2_cent > '9') return -1;
			importo = importo_intero*100 + (cifra1_cent-'0')*10 + (cifra2_cent-'0');
			break;
	}

	return importo;
}

// NOT_SIGNED, NO_COMMAND_FOUND, CREDENTIAL_TOO_LONG, NO_ERROR, USER_ALREADY_TAKEN
void show_error(enum ERROR e){
	const char *msg = "\0";
	switch(e){
		case NO_ERROR:
		msg = "";
		break;
		case NOT_LOGGED_IN:
		msg = "Utente non loggato.\n";
		break;
		case NO_COMMAND_FOUND:
		msg = "Comando non riconosciuto (digita !help per la lista di comandi)\n";
		break;
		case WRONG_CREDENTIALS_SIZE:
		msg = "Il nome o/e la password sono troppi lunghi o troppo corti (max 50 caratteri min 4).\n";
		break;
		case USER_ALREADY_TAKEN:
		msg = "Il nome utente scelto e` gia` in uso.\n";
		break;
		case WRONG_CREDENTIALS:
		msg = "Nome utente e/o password sbagliati\n";
		break;
		case BAD_REQUEST:
		msg = "Error: Bad Request to Server\n";
		break;
		case BANNED:
		msg = "Numero di tentativi esaurito, riprova piu` tardi.\n";
		break;
		case WRONG_SESSID:
		msg = "Error: Wrong Session ID\n";
		break;
		case SYNTAX_ERROR:
		msg = "Errore di sintassi: il comando non era ben composto.\n";
		break;
		case ALREADY_LOGGED:
		msg = "Utente gia` connesso.\n";
		break;
		case THREAD_LOGGED_IN:
		msg = "L'utente specificato risulta già connesso.\n";
		break;
		case DISCONNECTED:
		msg = "La connessione e' venuta a mancare\n";
		break;
		case SERVER_ERROR:
		msg = "Il server ha riscontrato un errore\n";
		break;
		default:
		msg = "errore non gestito\n";
		break;
	}
	printf("%s", msg);
	fflush(stdout);
}

// HELP, SIGNUP, LOGIN, INVIA_GIOCATA, VEDI_GIOCATA, VEDI_ESTRAZIONE, VEDI_VINCITE, ESCI
enum COMMAND str2command(char *_string){
	if (strcmp("!help", _string) == 0)
		return HELP;

	if (strcmp("!signup", _string) == 0)
		return SIGNUP;

	if (strcmp("!login", _string) == 0)
		return LOGIN;

	if (strcmp("!invia_giocata", _string) == 0)
		return INVIA_GIOCATA;

	if (strcmp("!vedi_giocate", _string) == 0)
		return VEDI_GIOCATE;

	if (strcmp("!vedi_estrazioni", _string) == 0)
		return VEDI_ESTRAZIONI;

	if (strcmp("!vedi_vincite", _string) == 0)
		return VEDI_VINCITE;

	if (strcmp("!esci", _string) == 0)
		return ESCI;

	return NO_COMMAND;
}

//BARI, CAGLIARI, FIRENZE, GENOVA, MILANO, NAPOLI, PALERMO, ROMA, TORIO, VENEZIA, NAZIONALE, TUTTE
enum RUOTA str2ruota(char *_string){
	if (strcmp("bari", _string) == 0)
		return BARI;

	if (strcmp("cagliari", _string) == 0)
		return CAGLIARI;

	if (strcmp("firenze", _string) == 0)
		return FIRENZE;

	if (strcmp("genova", _string) == 0)
		return GENOVA;

	if (strcmp("milano", _string) == 0)
		return MILANO;

	if (strcmp("napoli", _string) == 0)
		return NAPOLI;

	if (strcmp("palermo", _string) == 0)
		return PALERMO;

	if (strcmp("roma", _string) == 0)
		return ROMA;

	if (strcmp("torio", _string) == 0)
		return TORIO;

	if (strcmp("venezia", _string) == 0)
		return VENEZIA;

	if (strcmp("nazionale", _string) == 0)
		return NAZIONALE;

	if (strcmp("tutte", _string) == 0)
		return TUTTE;

	return NO_RUOTA;
}

//BARI, CAGLIARI, FIRENZE, GENOVA, MILANO, NAPOLI, PALERMO, ROMA, TORIO, VENEZIA, NAZIONALE, TUTTE
const char* ruota2str(enum RUOTA _r){
	switch (_r) {
		case BARI: return "Bari";
		case CAGLIARI: return "Cagliari";
		case FIRENZE: return "Firenze";
		case GENOVA: return "Genova";
		case MILANO: return "Milano";
		case NAPOLI: return "Napoli";
		case PALERMO: return "Palermo";
		case ROMA: return "Roma";
		case TORIO: return "Torio";
		case VENEZIA: return "Venezia";
		case NAZIONALE: return "Nazionale";
		default: return "";
	}
}
