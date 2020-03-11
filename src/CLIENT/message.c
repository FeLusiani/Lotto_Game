#include "message.h"

// NOT_SIGNED, NO_COMMAND_FOUND, CREDENTIAL_TOO_LONG, NO_ERROR, USER_ALREADY_TAKEN
void show_error(enum ERROR e){
	char *msg = "\0";
	switch(e){
		case NO_ERROR:
		msg = "";
		break;
		case NOT_SIGNED:
		msg = "non puoi accedere a queste funzioni, accedi o iscriviti prima.\n";
		break;
		case NO_COMMAND_FOUND:
		msg = "comando non riconosciuto (digita !help per la lista di comandi)\n";
		break;
		case CREDENTIAL_WRONG_SIZE:
		msg = "il nome o/e la password sono troppi lunghi o troppo corti (max 50 caratteri min 4).\n";
		break;
		case USER_ALREADY_TAKEN:
		msg = "Mi dispiace il nome utente scelto viene gia utilizzato.\n";
		break;
		case CREDENTIAL_NOT_CORRECT:
		msg = "nome utente e/o password sbagliati\n";
		break;
		case BAD_FORMAT:
		msg = "il formato dell'istruzione inviata era sbagliato (controlla help)\n";
		break;
		case BANNED:
		msg = "hai fatto troppi login riprovare tra un po'\n";
		break;
		case WRONG_SESSID:
		msg = "non sei autorizzato ad usare questa connessione\n";
		break;
		case SYNTAX_ERROR:
		msg = "errore di sintassi non hai inserito i dati in modo sintatticamente corretto\n";
		break;
		case ALREADY_LOGGED:
		msg = "sei già connesso con un utente\n";
		break;
		case THREAD_LOGGED_IN:
		msg = "mi dispiace ma questo utente risulta già connesso\n";
		break;
		case DISCONNECTED:
		msg = "non riesco a raggiungere il server\n";
		break;
		default:
		msg = "errore non gestito\n";
		break;
	}
	printf("%s", msg);
}

//HELP, SIGNUP, LOGIN, INVIA_GIOCATA, VEDI_GIOCATA, VEDI_ESTRAZIONE, VEDI_VINCITE, ESCI, NO_COMMAND
void show_help(enum COMMAND c){
	char *str, *help, *signup, *login, *invia_giocata, *vedi_giocata, *vedi_estrazione, *vedi_vincite, *esci, *basic;
    basic =           "*************************** BENVENUTO AL GIOCO DEL LOTTO ***************************\n"
                      "Sono disponibili i seguenti comandi:\n\n"
                      "1) !help <comando> : mostra i dettagli di un comando\n"
                      "2) !signup <username> <password> : crea un nuovo utente\n"
                      "3) !login <username> <password> : autentica un utente precedentemente iscritto\n"
                      "4) !invia_giocata <giocata> : invia una giocata al server\n"
                      "5) !vedi_giocata <tipo> : vede le giocate di un determinato tipo \n"
                      "6) !vedi_estrazione <n> <ruota> : mostra le precedenti n estrazioni su una\n determinata ruota\n"
	                  "7) !esci : effettua il logout dell utente\n";
    help =            "-!help <comando> : mostra i dettagli di un determinato <comando>\n";
    signup =          "-!signup <username> <password> : crea un nuovo utente <username> <password> possono\n"
                      "   utilizzare ogni carattere ascii tranne i caratteri speciali e devono essere lunghi\n"
                      "   piu' di 4 caratteri e meno di 50\n";
    login =           "-!login <username> <password> : autentica un utente precedentemente iscritto, ATTENZIONE:\n"
                      "   dopo 4 tentativi errati bisogna aspettare 30 secondi per una nuova iscrizione";
    invia_giocata =   "-!invia_giocata <giocata> : invia una giocata al server la giocata deve essere nella\n"
                      "   nella forma -r <ruote> -n <numeri> -i <valori> le ruote sono quelle standard piu'\n"
                      "   l'opzione 'tutte' i numeri devono essere da 1 a 5 interi (i numeri che si vogliono\n"
                      "   giocare) e i valori sono un insieme di valori numerici con al piu' deu cifre decimali\n"
                      "   che indicano la scommesso sul uscita rispettivamente di ESTRATTO, AMBO, TERNA, QUATERNA,\n"
                      "   CINQUINA, il numero di valori deve essere uguali a quello di numeri (mettere 0 se non si\n"
                      "   vuole scommetere)\n";
    vedi_giocata =    "-!vedi_giocata <tipo> : se <tipo> = 1 mostra le giocate attive ovvero quello di cui non si sa\n"
                      "   ancora il risultato se <tipo> = 0 mostra le giocate effettuate di cui si conosce il risultato\n";
    vedi_estrazione = "-!vedi_estrazione <n> <routa> : mostra le n estrazioni su una specifica ruota (mettere tutte oppure\n"
                      "   ingorare il campo se si vogliono di tutte le ruote)\n";
    vedi_vincite =    "-!vedi_vincite : mostra tutte le vincite dell'utente\n";
    esci =            "-!esci : effettua il logout dell utente\n";
	switch(c){
		case HELP:
		str = help;
		break;
		case SIGNUP:
		str = signup;
		break;
		case LOGIN:
		str = login;
		break;
		case INVIA_GIOCATA:
		str = invia_giocata;
		break;
		case VEDI_GIOCATA:
		str = vedi_giocata;
		break;
		case VEDI_ESTRAZIONE:
		str = vedi_estrazione;
		break;
		case VEDI_VINCITE:
		str = vedi_vincite;
		break;
		case ESCI:
		str = esci;
		break;
		default:
		str = basic;
		break;
	}
	printf("%s", str);
}
