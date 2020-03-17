#include "make_response.h"

// 1 banned, 0 not banned
void ban_ip(struct in_addr ip){
    char* ip_string = inet_ntoa(ip);
    char ban_time_string[50];
    time_t now = time(NULL);
    strftime(ban_time_string, sizeof(ban_time_string), "%d-%m-%Y %H:%M:%S", localtime(&now));

    FILE *file;
    file = fopen(BAN_FILE,"a");
    fprintf(file, "%s TIME=[%jd] %s\n", ip_string, now, ban_time_string);
    fclose(file);
}

// 1 banned, 0 not banned
int is_banned(struct in_addr ip){
    FILE *file;
    file = fopen(BAN_FILE,"r");
    char entry[100];
    char ip_string[INET_ADDRSTRLEN];
    time_t ban_time = 0;
    int is_banned_ = 0;
    while(fgets(entry,sizeof(entry),file) != NULL){ // per ogni linea contenuta nel file
        if (entry[0] == '\n' || entry[0] == '#') continue; // linea vuota o di commento
        sscanf(entry, "%s TIME=[%jd]", ip_string, &ban_time);
        struct in_addr ip_i;
        inet_aton(ip_string, &ip_i);
        if (ip_i.s_addr != ip.s_addr) continue;
        if (difftime(time(NULL), ban_time) < BAN_TIME)
            is_banned_ = 1;
    }
    fclose(file);

    return is_banned_;
}

enum ERROR login(thread_slot* thread_data, char* req_ptr, char* res_ptr){
    if (strcmp(thread_data->user, "") != 0) return ALREADY_LOGGED;
    if (is_banned(thread_data->ip)) return BANNED;

    char user[51] = "";
    char pwd[51] = "";
    sscanf(req_ptr, "USER: %s\nPWD: %s", user, pwd);

    char usr_dir[MAX_PATH_LEN];
    sprintf(usr_dir, "%s/%s", USERS_DIR, user);
    // controllo se esiste gia' una directory a nome dell'utente
    struct stat st = {0};
    if (stat(usr_dir, &st) == -1){
        // autenticazione fallita
        thread_data->n_try ++;
        if (thread_data->n_try > 2){
            ban_ip(thread_data->ip);
            return BANNED;
        }
        else
            return WRONG_CREDENTIALS;
    }

    // cerco il file associato all'user
    char usr_filepath[MAX_PATH_LEN];
    sprintf(usr_filepath, "%s/pwd.txt", usr_dir);
    FILE *usr_file;
    usr_file = fopen(usr_filepath,"r");
    if (usr_file == NULL){
        printf("Could not open file: %s\n", usr_filepath);
        printf("%s\n", strerror(errno));
        return SERVER_ERROR;
    }
    char saved_pwd[51] = "";
    fscanf(usr_file,"%s",saved_pwd);
    fclose(usr_file);

    if (strcmp(pwd, saved_pwd) != 0){
        // autenticazione fallita
        thread_data->n_try ++;
        if (thread_data->n_try > 2){
            ban_ip(thread_data->ip);
            return BANNED;
        }
        else
            return WRONG_CREDENTIALS;
    }
    // segno l'utente loggato
    strcpy(thread_data->user,user);
    // genero la session ID
    const char* alphanum = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for(int i=0; i < SESS_ID_SIZE-1; i++)
        thread_data->session_id[i] = alphanum[rand() % (sizeof(alphanum)-1)];

    sprintf(res_ptr, "SESSION_ID: %s\n", thread_data->session_id);

    return NO_ERROR;
}

enum ERROR signup(thread_slot* thread_data, char* req_ptr, char* res_ptr){
    if (strcmp(thread_data->user, "") != 0)
        return ALREADY_LOGGED;
    char user[51] = "";
    char pwd[51] = "";
    sscanf(req_ptr, "USER: %s\nPWD: %s", user, pwd);

    char usr_dir[MAX_PATH_LEN];
    sprintf(usr_dir, "%s/%s", USERS_DIR, user);
    // controllo se esiste gia' una directory a nome dell'utente
    struct stat st = {0};
    if (stat(usr_dir, &st) != -1) return USER_ALREADY_TAKEN;

    // creo la directory dell'utente
    if (mkdir(usr_dir, 0700) == -1){
        printf("Could not create directory: %s\n", usr_dir);
        printf("%s\n", strerror(errno));
        return SERVER_ERROR;
    }

    // creo i file relativi all'utente
    char new_filepath[MAX_PATH_LEN];
    FILE *new_file;

    // file contenente la password
    sprintf(new_filepath, "%s/pwd.txt", usr_dir);
    new_file = fopen(new_filepath,"w");
    if (new_file == NULL){
        printf("Could not create file: %s\n", new_filepath);
        printf("%s\n", strerror(errno));
        return SERVER_ERROR;
    }
    fprintf(new_file,"%s",pwd);
    fclose(new_file);

    // file delle scommesse attive
    sprintf(new_filepath, "%s/active_bets.txt", usr_dir);
    new_file = fopen(new_filepath,"w");
    if (new_file == NULL){
        printf("Could not create file: %s\n", new_filepath);
        printf("%s\n", strerror(errno));
        return SERVER_ERROR;
    }
    fclose(new_file);

    // file delle scommesse passate
    sprintf(new_filepath, "%s/old_bets.txt", usr_dir);
    new_file = fopen(new_filepath,"w");
    if (new_file == NULL){
        printf("Could not create file: %s\n", new_filepath);
        printf("%s\n", strerror(errno));
        return SERVER_ERROR;
    }
    fclose(new_file);

    // file consuntivo (contiene le vincite comulative per ogni combo, in centesimi)
    sprintf(new_filepath, "%s/consuntivo.txt", usr_dir);
    new_file = fopen(new_filepath,"w");
    if (new_file == NULL){
        printf("Could not create file: %s\n", new_filepath);
        printf("%s\n", strerror(errno));
        return SERVER_ERROR;
    }
    fprintf(new_file,"0 0 0 0 0"); // scrivo i 5 valori iniziali
    fclose(new_file);

    return NO_ERROR;
}

enum ERROR invia_giocata(thread_slot* thread_data, char* req_ptr, char* res_ptr){
    if (strcmp(thread_data->user, "") == 0)
        return NOT_LOGGED_IN;
    // scrivo la scommessa nel file
    char filepath[MAX_PATH_LEN];
    sprintf(filepath, "%s/%s/active_bets.txt", USERS_DIR, thread_data->user);
    FILE *file;
    file = fopen(filepath,"a");
    fprintf(file,"%s",req_ptr);
    fclose(file);
    return NO_ERROR;
}

// copia il file [filename] appartenente all'user [user] nel buffer di risposta puntato da [res_ptr_p]
// il puntatore del buffer di risposta viene incrementato dall'operazione
enum ERROR print_user_file(const char* _filename,const char* _user, char** res_ptr_p){
    char filepath[MAX_PATH_LEN];
    FILE *file;
    sprintf(filepath, "%s/%s/%s", USERS_DIR, _user, _filename);
    file = fopen(filepath,"r");
    if (file == NULL){
        printf("Could not open file: %s\n", filepath);
        printf("%s\n", strerror(errno));
        return SERVER_ERROR;
    }
    // copio il file nella risposta
    char c;
    while ( ( c = fgetc(file) ) != EOF)
    {
        (*res_ptr_p)[0] = c;
        (*res_ptr_p) ++;
    }
    fclose(file);
    return NO_ERROR;
}

enum ERROR vedi_giocate(thread_slot* thread_data, char* req_ptr, char* res_ptr){
    if (strcmp(thread_data->user, "") == 0)
        return NOT_LOGGED_IN;
    char tipo;
    // leggo il tipo richiesto
    sscanf(req_ptr, "TIPO: %c\n", &tipo);

    // copio il file richiesto nel messaggio di risposta
    enum ERROR err = NO_ERROR;
    if (tipo == '0')
        err = print_user_file("old_bets.txt", thread_data->user, &res_ptr);
    else
        err = print_user_file("active_bets.txt", thread_data->user, &res_ptr);

    return err;
}

enum ERROR vedi_estrazioni(thread_slot* thread_data, char* req_ptr, char* res_ptr){
    if (strcmp(thread_data->user, "") == 0)
        return NOT_LOGGED_IN;

    int n_estr;
    enum RUOTA ruota;
    if (sscanf(req_ptr, "N_ESTRAZIONI: %d\nRUOTA: %d\n", &n_estr, (int*)&ruota) < 2)
        return BAD_REQUEST;

    if (n_estr <= 0) return NO_ERROR;

    // apro il file delle estrazioni
    FILE *file;
    file = fopen(ESTRAZ_FILE,"r");
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file); //lunghezza del file
    int num_estrazioni_totali = length / 776; // ogni estrazione occupa 776 caratteri nel file
    if (n_estr > num_estrazioni_totali){
        sprintf(res_ptr, "Numero indicato troppo grande. In memoria sono presenti %d estrazioni.\n", num_estrazioni_totali);
        fclose(file);
        return NO_ERROR;
    }

    size_t chars_to_read = n_estr*776;
    fseek(file, -chars_to_read, SEEK_END); // mi preparo a leggere le ultime n_estr estrazioni

    if (ruota == TUTTE){
        // copia tutto
        char c;
        while ( ( c = fgetc(file) ) != EOF)
        {
            res_ptr[0] = c;
            res_ptr ++;
        }
        res_ptr[0] = '\0';
    }
    else{
        // copia solo le righe della ruota indicata nel comando
        // la prima riga dell'estrazione contiene 38 caratteri + \n
        // seguono N_RUOTE righe di  66 caratteri + \n ciascuna
        fseek(file, 39 + 67*(int)ruota, SEEK_CUR); // vado alla prima riga di interesse
        while (fgets(res_ptr, 67 + 1, file)){
            res_ptr += 67;
            fseek(file, 776-67,SEEK_CUR);
        }
    }
    return NO_ERROR;

}

enum ERROR vedi_vincite(thread_slot* thread_data, char* req_ptr, char* res_ptr){
    if (strcmp(thread_data->user, "") == 0)
        return NOT_LOGGED_IN;
    // copio il file vincite.txt nel messaggio di risposta
    enum ERROR err = NO_ERROR;
    err = print_user_file("vincite.txt", thread_data->user, &res_ptr);
    if (err != NO_ERROR) return err;
    // stampo il consuntivo
    char tmp[100];
    char* tmp_ptr = &tmp[0];
    // leggo il file consuntivo dell'user
    err = print_user_file("consuntivo.txt", thread_data->user, &tmp_ptr);
    if (err != NO_ERROR) return err;

    long int val_file[N_COMBO]; // in centesimi
    sscanf(tmp, "%ld %ld %ld %ld %ld", &val_file[0], &val_file[1], &val_file[2], &val_file[3], &val_file[4]);
    const char* combo[N_COMBO];
    combo[0] = "ESTRATTO";
    combo[1] = "AMBO";
    combo[2] = "TERNO";
    combo[3] = "QUATERNA";
    combo[4] = "CINQUINA";

    sprintf(res_ptr, "\n");
    res_ptr = next_line(res_ptr);
    for (int i=0; i<N_COMBO; i++){
        sprintf(res_ptr, "Vincite su %s: %.2f\n", combo[i], val_file[i] / 100.0);
        res_ptr = next_line(res_ptr);
    }

    return NO_ERROR;
}

enum ERROR esci(thread_slot* thread_data, char* req_ptr, char* res_ptr){
    return NO_ERROR;
}

enum ERROR make_response(thread_slot* thread_data, enum COMMAND command, char* msg_ptr, char*res_ptr){
    enum ERROR e_;
    switch(command){
        case LOGIN:
            e_ = login(thread_data, msg_ptr, res_ptr);
            break;
        case SIGNUP:
            e_ = signup(thread_data, msg_ptr, res_ptr);
            break;
        case INVIA_GIOCATA:
            e_ = invia_giocata(thread_data, msg_ptr, res_ptr);
            break;
        case VEDI_GIOCATE:
            e_ = vedi_giocate(thread_data, msg_ptr, res_ptr);
            break;
        case VEDI_ESTRAZIONI:
            e_ = vedi_estrazioni(thread_data, msg_ptr, res_ptr);
            break;
        case VEDI_VINCITE:
            e_ = vedi_vincite(thread_data, msg_ptr, res_ptr);
            break;
        case ESCI:
            e_ = esci(thread_data, msg_ptr, res_ptr);
            break;
        default:
            e_ = NO_COMMAND_FOUND;
            break; //riprova a leggere l'input
    }

    return e_;
}
