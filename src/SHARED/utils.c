#include "utils.h"

/*
	trasforma una stringa in un intero... restituisce l'intero come le prime cifre decimali che trova
	se individua una cifra non decimale esce inoltre inserisce in num_ la posizione dell'ultimo valore
	decimale letto. (Ex: _str = "23a" ---> return 23; num_ = 1 _str[num_] = '3')
*/
int get_int_from_string(char* _str, int* num_){
	int res_ = 0;
	for(*num_ = 0; _str[*num_] != '\0' && _str[*num_] >= '0' && _str[*num_] <= '9'; (*num_) = (*num_) + 1){
		res_ = 10 * res_ + (int)(_str[*num_] - '0');
	}
	*num_ = *num_ - 1;
	return res_;
}

/* ritorna 1 se r fa parte dell enum RUOTA altrimenti 0 */
int isRuota(enum RUOTA r){
	return  r == BARI || r == CAGLIARI || r == FIRENZE || r == GENOVA || r == MILANO || r == NAPOLI || r == PALERMO || r == ROMA || r == TORIO || r == VENEZIA || r == NAZIONALE || r == TUTTE || r == NO_RUOTA;
}

// HELP, SIGNUP, LOGIN, INVIA_GIOCATA, VEDI_GIOCATA, VEDI_ESTRAZIONE, VEDI_VINCITE, ESCI

/* legge in caratteri un comanda e restituisce il comando sotto forma di enum */ 
enum COMMAND str2command(char *_c){
	struct str_command{
		char* str;
		enum COMMAND command;
		
	};
	struct str_command corrispondences[] = {
		{"!help",            HELP           },
		{"!signup",          SIGNUP         },
		{"!login",           LOGIN          },
		{"!invia_giocata",   INVIA_GIOCATA  },
		{"!vedi_giocata",    VEDI_GIOCATA   },
		{"!vedi_estrazione", VEDI_ESTRAZIONE},
		{"!vedi_vincita",    VEDI_VINCITE   },
		{"!esci",            ESCI           }
	};
	int i;
	for(i = 0; i < N_COMMANDS; i++){
		if(strcmp(corrispondences[i].str, _c) == 0)
			return corrispondences[i].command;
	}
	return  NO_COMMAND;
}

//BARI, CAGLIARI, FIRENZE, GENOVA, MILANO, NAPOLI, PALERMO, ROMA, TORIO, VENEZIA, NAZIONALE, TUTTE
enum RUOTA str2ruota(char *_c){
	struct str_ruota{
		char* str;
		enum RUOTA ruota;
		
	};
	struct str_ruota corrispondences[] = {
		{"bari",      BARI     },
		{"cagliari",  CAGLIARI },
		{"firenze",   FIRENZE  },
		{"genova",    GENOVA   },
		{"milano",    MILANO   },
		{"napoli",    NAPOLI   },
		{"palermo",   PALERMO  },
		{"roma",      ROMA     },
		{"torio",     TORIO    },
		{"venezia",   VENEZIA  },
		{"nazionale", NAZIONALE},
		{"tutte",     TUTTE    }
	};
	int i;
	for(i = 0; i < N_RUOTE + 2; i++){
		if(strcmp(corrispondences[i].str, _c) == 0)
			return corrispondences[i].ruota;
	}
	return  NO_RUOTA;
}

//BARI, CAGLIARI, FIRENZE, GENOVA, MILANO, NAPOLI, PALERMO, ROMA, TORIO, VENEZIA, NAZIONALE, TUTTE
const char* ruota2str(enum RUOTA _r){
	struct str_ruota{
		char* str;
		enum RUOTA ruota;
		
	};
	struct str_ruota corrispondences[] = {
		{"bari",      BARI     },
		{"cagliari",  CAGLIARI },
		{"firenze",   FIRENZE  },
		{"genova",    GENOVA   },
		{"milano",    MILANO   },
		{"napoli",    NAPOLI   },
		{"palermo",   PALERMO  },
		{"roma",      ROMA     },
		{"torio",     TORIO    },
		{"venezia",   VENEZIA  },
		{"nazionale", NAZIONALE},
		{"tutte",     TUTTE    }
	};
	int i;
	for(i = 0; i < N_RUOTE + 2; i++){
		if(corrispondences[i].ruota == _r)
			return corrispondences[i].str;
	}
	return  "";
}

/* mostra un valore double fino a 2 valori decimali */
void show_double(double val){
	printf("%d", (int) val);
	if((((int)(100 * val)) != 100 * ((int)val)))
		printf(".%d", (((int)(100 * val)) - 100 * ((int)val)));
}

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
