#include "make_response.h"
#define MAX_PATH_LEN 100
#define BAN_TIME 30 * 60

const char* REG_DIR = "REGISTRO";
const char* BAN_FILE = "REGISTRO/ip_bannati.txt";

// 1 banned, 0 not banned
void ban_ip(struct in_addr ip){
    char* ip_string = inet_ntoa(ip);
    char ban_time_string[50];
    time_t now = time(NULL);
    strftime(ban_time_string, sizeof(ban_time_string), "%Y-%m-%d %H:%M:%S", localtime(&now));

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

enum ERROR login(thread_slot* thread_data, char* req_ptr, char* res_buf){
    if (strcmp(thread_data->user, "") != 0) return ALREADY_LOGGED;
    if (is_banned(thread_data->ip)) return BANNED;

    char user[51] = "";
    char pwd[51] = "";
    sscanf(req_ptr, "USER: %s\nPWD: %s", user, pwd);

    char usr_dir[MAX_PATH_LEN];
    sprintf(usr_dir, "%s/%s/%s", REG_DIR, "users", user);
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
    // genero la session ID
    const char* alphanum = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for(int i=0; i < SESS_ID_SIZE-1; i++)
        thread_data->session_id[i] = alphanum[rand() % (sizeof(alphanum)-1)];

    sprintf(res_buf, "SESSION_ID: %s\n", thread_data->session_id);

    return NO_ERROR;
}

enum ERROR signup(thread_slot* thread_data, char* req_ptr, char* res_buf){
    if (strcmp(thread_data->user, "") != 0)
        return ALREADY_LOGGED;
    char user[51] = "";
    char pwd[51] = "";
    sscanf(req_ptr, "USER: %s\nPWD: %s", user, pwd);

    char usr_dir[MAX_PATH_LEN];
    sprintf(usr_dir, "%s/%s/%s", REG_DIR, "users", user);
    // controllo se esiste gia' una directory a nome dell'utente
    struct stat st = {0};
    if (stat(usr_dir, &st) != -1) return USER_ALREADY_TAKEN;

    strcpy(thread_data->user, user);

    // creo la directory dell'utente
    if (mkdir(usr_dir, 0700) == -1){
        printf("Could not create directory: %s\n", usr_dir);
        printf("%s\n", strerror(errno));
        return SERVER_ERROR;
    }

    // creo il file associato all'user
    char usr_filepath[MAX_PATH_LEN];
    sprintf(usr_filepath, "%s/pwd.txt", usr_dir);
    FILE *usr_file;
    usr_file = fopen(usr_filepath,"w");
    if (usr_file == NULL){
        printf("Could not create file: %s\n", usr_filepath);
        printf("%s\n", strerror(errno));
        return SERVER_ERROR;
    }
    fprintf(usr_file,"%s",pwd);
    fclose(usr_file);

    // creo la directory delle scommesse attive
    char user_active_bets[MAX_PATH_LEN];
    sprintf(user_active_bets, "%s/active_bets", usr_dir);
    if (mkdir(user_active_bets, 0700) == -1){
        printf("Could not create directory: %s\n", user_active_bets);
        printf("%s\n", strerror(errno));
        return SERVER_ERROR;
    }

    // creo la directory delle scommesse attive
    char user_old_bets[MAX_PATH_LEN];
    sprintf(user_old_bets, "%s/old_bets", usr_dir);
    if (mkdir(user_old_bets, 0700) == -1){
        printf("Could not create directory: %s\n", user_old_bets);
        printf("%s\n", strerror(errno));
        return SERVER_ERROR;
    }

    return NO_ERROR;
}

enum ERROR invia_giocata(thread_slot* thread_data, char* req_ptr, char* res_buf){
    return NO_ERROR;
}


enum ERROR vedi_giocata(thread_slot* thread_data, char* req_ptr, char* res_buf){
    return NO_ERROR;
}

enum ERROR vedi_estrazione(thread_slot* thread_data, char* req_ptr, char* res_buf){
    return NO_ERROR;
}

enum ERROR vedi_vincite(thread_slot* thread_data, char* req_ptr, char* res_buf){
    return NO_ERROR;
}

enum ERROR esci(thread_slot* thread_data, char* req_ptr, char* res_buf){
    return NO_ERROR;
}

enum ERROR make_response(thread_slot* thread_data, enum COMMAND command, char* msg_ptr, char*res_buf){
    enum ERROR e_;
    switch(command){
        case LOGIN:
            e_ = login(thread_data, msg_ptr, res_buf);
            break;
        case SIGNUP:
            e_ = signup(thread_data, msg_ptr, res_buf);
            break;
        case INVIA_GIOCATA:
            e_ = invia_giocata(thread_data, msg_ptr, res_buf);
            break;
        case VEDI_GIOCATA:
            e_ = vedi_giocata(thread_data, msg_ptr, res_buf);
            break;
        case VEDI_ESTRAZIONE:
            e_ = vedi_estrazione(thread_data, msg_ptr, res_buf);
            break;
        case VEDI_VINCITE:
            e_ = vedi_vincite(thread_data, msg_ptr, res_buf);
            break;
        case ESCI:
            e_ = esci(thread_data, msg_ptr, res_buf);
            break;
        default:
            e_ = NO_COMMAND_FOUND;
            break; //riprova a leggere l'input
    }

    return e_;
}
