#include "utils.h"

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

	if (strcmp("!vedi_giocata", _string) == 0)
		return VEDI_GIOCATA;

	if (strcmp("!vedi_estrazione", _string) == 0)
		return VEDI_ESTRAZIONE;

	if (strcmp("!vedi_vincita", _string) == 0)
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


/* ritorna la lunghezza della stringa e setta a 1 flag se non trova il carattere '\0' prima di max */
int strlenbelow(char* str, int max, int *flag){
	int i;
	*flag = 0;
	for(i = 0; i < max && str[i] != '\0'; i++){};
	if(i == max - 1 && str[i] != '\0')
		*flag = 1;
	return i;
}

/* calcola le disposizioni di N,k calcloate come D(N, k) = N! / (N - k)! */
int disposizioni(int N, int k){
	int i, res = 1;
	for(i = N - k + 1; i <= N; i++)
		res *= i;
	return res;
}
