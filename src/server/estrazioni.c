#include "estrazioni.h"

// estrae la prima giocata contenuta nel file puntato da giocate_f
// restituisce 0 se non riesce ad eseguire la lettura
int parse_user_bets(FILE* giocate_f, char r_giocate_[N_RUOTE+1], int numeri_[10], int importi_[N_COMBO]){
    const char* format_giocata;
    format_giocata = "RUOTE_GIOCATE: %s\nNUMERI_GIOCATI:%[^\n]\nIMPORTI:%[^\n]\n";
    char numeri_str[50];
    char importi_str[100];
    int res = fscanf(giocate_f, format_giocata, r_giocate_, numeri_str, importi_str);

    if (res < 3) return 0;

    int i;
    // inizializzo a 0 i numeri giocati
    for (i=0; i<10; i++) numeri_[i] = 0;
    // estrapolo i numeri giocati
    int chars_read = 0;
    char* n_str_ptr = &numeri_str[0];
    i = 0;
    while (sscanf(n_str_ptr, "%d%n", &numeri_[i], &chars_read) == 1){
        n_str_ptr += chars_read;
        i++;
    }

    // inizializzo a 0 i numeri giocati
    for (i=0; i<5; i++) importi_[i] = 0;
    // estrapolo gli importi giocati
    chars_read = 0;
    char* imp_str_ptr = &importi_str[0];
    i = 0;
    while (sscanf(imp_str_ptr, "%d%n", &importi_[i], &chars_read) == 1){
        imp_str_ptr += chars_read;
        i++;
    }

    return 1;
}

// appende il contenuto del file puntato da a_bets_fptr al file old_bets.txt dell'user
void archivia_scommesse(FILE* a_bets_fptr, char* user){
    // apro il file old_bets
    char filepath[MAX_PATH_LEN];
    sprintf(filepath, "%s/%s/old_bets.txt", USERS_DIR, user);
    FILE* o_bets_fptr = fopen(filepath,"a");

    // copio a_bets_fptr in o_bets_fptr
    fseek(a_bets_fptr, 0, SEEK_SET);
    char c;
    while( ( c = fgetc(a_bets_fptr) ) != EOF )
       fputc(c, o_bets_fptr);

    fclose(o_bets_fptr);
}

// calcola le disposizioni di k elementi su N
// D(N, k) = N! / (N - k)!
int disposizioni(int N, int k){
	int i, res = 1;
	for(i = N - k + 1; i <= N; i++)
		res *= i;
	return res;
}

// aggiorna il file consuntivo dell'user
// sommando il vettore di guadagni riportati per tipo di combo a quello presente nel file
void aggiorna_consuntivo(double vincite[N_COMBO], char* user){
    long int val_file[N_COMBO]; // in centesimi
    // leggo il file consuntivo.txt dell'user
    char filepath[MAX_PATH_LEN];
    sprintf(filepath, "%s/%s/consuntivo.txt", USERS_DIR, user);
    FILE* file = fopen(filepath,"r+");
    fscanf(file,"%ld %ld %ld %ld %ld", &val_file[0], &val_file[1], &val_file[2], &val_file[3], &val_file[4]);
    // sommo le vincite appena fatte a quelle lette
    for (int i=0; i<N_COMBO; i++) val_file[i] += (int) (vincite[i]*100);
    // salvo nel file
    fseek(file, 0, SEEK_SET);
    fprintf(file,"%ld %ld %ld %ld %ld", val_file[0], val_file[1], val_file[2], val_file[3], val_file[4]);
    fclose(file);
}

// applico l'estrazione alle scommesse attive degli utenti
// numeri_estratti -> numeri dell'estrazione
// time_string -> stringa contenente il timestamp dell'estrazione
void apply_estrazione(int numeri_estratti[N_RUOTE][5], const char* _time_string){
    // per ogni user, applico le scommesse attive
    DIR * users_dir = opendir(USERS_DIR);
    struct dirent *u_dir;
    while( (u_dir=readdir(users_dir)) )
    {
        if (strcmp(u_dir->d_name, ".") == 0 || strcmp(u_dir->d_name, "..") == 0)
            continue; // salta questi due directory
        char user[50];
        strcpy(user, u_dir->d_name);
        char filepath[MAX_PATH_LEN];

        int vincita = 0; // 1 se in questa estrazione l'utente ha fatto una vincita
        double vincite[N_COMBO]; // soldi vinti sul rispettivo tipo di combo
        for (int i=0; i<N_COMBO; i++) vincite[i] = 0;

        // apro il file delle scommesse
        sprintf(filepath, "%s/%s/active_bets.txt", USERS_DIR, user);
        FILE *scommesse_f;
        scommesse_f = fopen(filepath,"r");
        // apro il file delle vincite
        sprintf(filepath, "%s/%s/vincite.txt", USERS_DIR, user);
        FILE *vincite_f;
        vincite_f = fopen(filepath,"a");

        if (scommesse_f == NULL || vincite_f == NULL){
            printf("Could not open files for user: %s\n", user);
            printf("%s\n", strerror(errno));
            continue;
        }

        // leggo ed applico ogni giocata contenuta nel file
        char ruote_giocate[N_RUOTE+1];
        int num_giocati[10];
        int importi[N_COMBO];
        while(parse_user_bets(scommesse_f, ruote_giocate, num_giocati, importi)){
            int indovinati_counter = 0; // numero dei numeri indovinati
            // per ogni numero giocato
            for (int i=0; i<10; i++){
                int trovato = 0;
                // per ogni ruota giocata
                for (int j=0; j<N_RUOTE; j++){
                    if (ruote_giocate[j] == '-') continue; // ruota non giocata
                    // cerco il numero giocato tra i 5 di questa ruota
                    for (int z=0; z<5; z++){
                        if(num_giocati[i] == numeri_estratti[j][z]){
                            indovinati_counter ++; // numero indovinato!
                            trovato = 1;
                            break;
                        }
                        if (trovato) break;
                    }
                    if (trovato) break;
                }
            }


            if (indovinati_counter == 0) continue;

            if (vincita == 0){ // prima giocata dell'estrazione a causare una vincita
                fprintf(vincite_f, "Estrazione del %s\n", _time_string);
                vincita = 1; // setto a 1 vincita
            }
            // STAMPO LA GIOCATA
            // stampa (e conta) le ruote
            int r_giocate_counter = 0;
            if (strcmp(ruote_giocate, "XXXXXXXXXXX") == 0){
                fprintf(vincite_f," tutte");
                r_giocate_counter = N_RUOTE;
            }
            else{
                for (int i=0; i<N_RUOTE; i++){
                    if (ruote_giocate[i] == '-') continue;
                    enum RUOTA r = (enum RUOTA ) i;
                    fprintf(vincite_f," %s", ruota2str(r));
                    r_giocate_counter ++;
                }
            }

            fprintf(vincite_f, "   ");
            // stampo (e conto) i numeri giocati
            int n_giocati_counter = 0;
            for (int i=0; i<10 && num_giocati[i] != 0; i++){
                fprintf(vincite_f, " %d", num_giocati[i]);
                n_giocati_counter ++;
            }

            fprintf(vincite_f, " >> ");
            // CALCOLO LE VINCITE
            double valore[N_COMBO]; // valore della combo
            valore[0] = 11.23;
            valore[1] = 250;
            valore[2] = 4500;
            valore[3] = 120000;
            valore[4] = 6000000;
            const char* combo[N_COMBO];
            combo[0] = "Estratto";
            combo[1] = "Ambo";
            combo[2] = "Terno";
            combo[3] = "Quaterna";
            combo[4] = "Cinquina";

            for (int i=N_COMBO-1; i>=0; i--){
                if (importi[i] == 0) continue;
                fprintf(vincite_f, " %s", combo[i]);
                if (indovinati_counter < (i+1) ){
                    // non ho attivato la combo di ordine i
                    fprintf(vincite_f, " 0");
                    continue;
                }

                int comb_possibili = disposizioni(n_giocati_counter, i+1);
                int comb_realizzate = disposizioni(indovinati_counter, i+1);
                double guadagno = (double) importi[i] / 100.0; // gli importi sono in centesimi
                guadagno *= valore[i];
                guadagno *= comb_realizzate / (double) comb_possibili;
                guadagno /= (double) r_giocate_counter;

                fprintf(vincite_f, " %.2f", guadagno);
                vincite[i] += guadagno;
                break; // non importano le combo di ordine inferiore
            }

            fprintf(vincite_f, "\n");
        }
        if (vincita == 1){ // l'user ha riscontrato almeno una vincita
            fprintf(vincite_f, "*******************************************\n");
            aggiorna_consuntivo(vincite, user);
        }

        archivia_scommesse(scommesse_f, user);
        fclose(scommesse_f);
        // ri-apro il file delle scommesse per svuotarlo
        sprintf(filepath, "%s/%s/active_bets.txt", USERS_DIR, user);
        scommesse_f = fopen(filepath,"w");

        fclose(scommesse_f);
        fclose(vincite_f);
    }
    closedir(users_dir);
}

/*
Crea un'estrazione, applica le scommesse attive degli utenti,
salva l'estrazione nel file delle estrazioni e aggiorna il file delle vincite per ogni utente
*/

void make_estrazione(){
	// apro il file delle estrazioni
	FILE *estraz_f;
	estraz_f = fopen(ESTRAZ_FILE,"a");

	int numeri_estratti[N_RUOTE][5];
	time_t now = time(NULL);
	char time_string[50];
    strftime(time_string, sizeof(time_string), "%d-%m-%Y ore %H:%M:%S", localtime(&now));
	fprintf(estraz_f, "Estrazione del %s\n", time_string);
	printf("Estrazione del %s\n", time_string);

	int numeri_possibili[90]; // array dei numeri che possono essere estratti
	for(int i = 0; i <  90; i++) numeri_possibili[i] = i + 1;
	// per ogni ruota estraggo 5 numeri
	for (int i=0; i<N_RUOTE; i++){
		int chars_written = 0;
		fprintf(estraz_f, " %-15s%n", ruota2str( (enum RUOTA) i), &chars_written); // stampo la ruota
		printf(" %-15s%n", ruota2str( (enum RUOTA) i), &chars_written);

		for (int j=0; j<5; j++){
			int index = rand() % (90 - j); // un index a caso per scegliere tra i 90-j numeri che posso ancora estrarre
			int estratto = numeri_possibili[index];
			numeri_estratti[i][j] = estratto;
			fprintf(estraz_f, "%-10d", estratto);
			printf("%-10d", estratto);

			// scambio il numero estratto con l'ultimo numero dell'array
			// in questo modo, all'interno di questo for non estratto gli stessi numeri
			numeri_possibili[index] = numeri_possibili[90 -1 -j];
			numeri_possibili[90 -1 -j] = estratto;
		}
		fprintf(estraz_f, "\n");
		printf("\n");
	}

	fclose(estraz_f);
	apply_estrazione(numeri_estratti, time_string);
}
