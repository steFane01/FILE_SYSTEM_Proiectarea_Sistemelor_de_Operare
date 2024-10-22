#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <signal.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>
#include <stdint.h>


#define ROOT_DIR "./files/"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define LOGS "logFile.txt"

sem_t sem_clienti;
pthread_mutex_t mutex_fisiere=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_logFile=PTHREAD_MUTEX_INITIALIZER;
volatile int stop_server = 0;


typedef struct{

    int nr_octeti_cuv;
    char cuv[25];
    int nr_oct_dim_lista;
    char lista_fisiere[4096];

}search_req_t;

typedef struct{
    int sock;
    search_req_t* search_req;
}search_req_start_t;

typedef struct{

    int status;
    int nr_oct_cale_fisier;
    char cale_fisier[30];
    int octet_start;
    int dim;
    char new_ch[4096];

}update_req_t;

typedef struct{
    int sock;
    update_req_t* update_req;
}update_req_start_t;

typedef struct{

    int status;
    int nr_octeti_cale_sursa;
    int nr_octeti_cale_destinatie;
    char content_sursa[4096];
    char cale_sursa[30];
    char cale_destinatie[30];

}move_req_t;

typedef struct{

    int sock;
    move_req_t* move_req;

}move_req_start_t;

typedef struct{

    uint32_t status;
    int nr_oct_cale_fisier;
    char cale_fisier[30];

}delete_req_t;

typedef struct{

    int sock;
   delete_req_t*delete_req; 

}delete_req_start_t;

typedef struct{

    uint32_t status;
    int nr_oct_cale_fisier;
    int nr_octeti_file_content;
    char cale_fisier[30];
    char file_content[4096];

}upload_req_t;
typedef struct{

    int sock;
    upload_req_t* upload_req;

}upload_req_start_t;


typedef struct{

    uint32_t status;
    char file_content[4096];
    int nr_octeti_file_content;
    int nr_octeti_cale_fisier;
    char cale_fisier[30];

}down_req_t;
typedef struct{
    int sock;
    down_req_t* down_req;
}down_req_start_t;

typedef struct{
    uint32_t status;
    int nr_octeti_rsapuns;
    char file_list[5000];
}list_req_t;

typedef struct{
    int sock;
    list_req_t* list_req;
}list_req_start_t;


typedef struct {
    int client_socket;
    pthread_t tid;
    uint32_t op_code;
} client_info_t;





void get_currenttime(char*buffer,size_t size){
    time_t t=time(NULL);
    struct tm *tm_info=localtime(&t);
    strftime(buffer,size,"%Y-%m-%d %H:%M:%S",tm_info);
}

void writeInLogs(const char*cod_op,char*path,char*word){

    char data_ora[20];
    get_currenttime(data_ora,sizeof(data_ora));

    char log_entry[256];
    snprintf(log_entry,sizeof(log_entry),"%s %s",data_ora,cod_op);

    if(path!=NULL){
        strncat(log_entry,", ",sizeof(log_entry)-strlen(log_entry)-1);
        strncat(log_entry,path,sizeof(log_entry)-strlen(log_entry)-1);
    }
    if(word!=NULL){
        strncat(log_entry,", ",sizeof(log_entry)-strlen(log_entry)-1);
        strncat(log_entry,word,sizeof(log_entry)-strlen(log_entry)-1);
    }

    pthread_mutex_lock(&mutex_logFile);

    FILE*logs=fopen(LOGS,"a");

    if(logs==NULL){
        perror("ERR open logFile\n");
        pthread_mutex_unlock(&mutex_logFile);
        return;
    }
    fprintf(logs,"%s\n",log_entry);

    fclose(logs);

    pthread_mutex_unlock(&mutex_logFile);

}

void* handle_signals(void* arg) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGINT);

    pthread_sigmask(SIG_BLOCK, &set, NULL);

    int sig;
    while (!stop_server) {
        sigwait(&set, &sig);

        if (sig == SIGTERM || sig == SIGINT) {
            printf("Semnal primit. Inițierea shutdown...\n");
            stop_server = 1;
            break;
        }
    }

    return NULL;
}

void* handle_list_operation(void* args) {
    list_req_start_t* lrs = (list_req_start_t*)args;
    lrs->list_req=(list_req_t*)malloc(sizeof(list_req_t));
    int pipe_fds[2];
    pid_t pid;

    if (pipe(pipe_fds) == -1) {
        perror("Eroare la crearea pipe-ului");
        lrs->list_req->status = 0x40;  
        pthread_exit(NULL);
    }

    pid = fork();
    if (pid < 0) {
        perror("Eroare la crearea procesului copil");
        lrs->list_req->status = 0x40;  
        pthread_exit(NULL);
    } else if (pid == 0) {
        // Proces copil - execuția comenzii `find ./files'
        close(pipe_fds[0]);  
        dup2(pipe_fds[1], STDOUT_FILENO);  
        close(pipe_fds[1]); 

        char *find_args[] = {"find", "./files/", NULL};
        execvp("find", find_args);

        perror("Execvp a eșuat");
        exit(1);  
    } else {
        close(pipe_fds[1]); 

        ssize_t bytes_read;
        bytes_read = read(pipe_fds[0], lrs->list_req->file_list, sizeof(lrs->list_req->file_list) - 1);

        if (bytes_read >= 0) {
            lrs->list_req->file_list[bytes_read] = '\0';  
            lrs->list_req->nr_octeti_rsapuns = bytes_read;  
            lrs->list_req->status = 0x0;  
        } else {
            perror("Eroare la citirea din pipe");
            lrs->list_req->status = 0x40;  
        }

        close(pipe_fds[0]);  

        int status;
        waitpid(pid, &status, 0);

        send(lrs->sock, lrs->list_req, sizeof(list_req_t), 0);

        free(lrs);
    }

    writeInLogs("0x0",NULL,NULL);

    pthread_exit(NULL);
}

void* handle_download_operation(void* args) {
    
    down_req_start_t* down_rs=(down_req_start_t*)args;
    pthread_mutex_lock(&mutex_fisiere);
    FILE*f=fopen(down_rs->down_req->cale_fisier,"rb");

    fseek(f,0,SEEK_END);
    down_rs->down_req->nr_octeti_file_content=ftell(f);
    rewind(f);
    fread(down_rs->down_req->file_content,1,down_rs->down_req->nr_octeti_file_content,f);

    fclose(f);
    pthread_mutex_unlock(&mutex_fisiere);
    char *buff;
    buff=strrchr(down_rs->down_req->cale_fisier,'/');
    buff++;
    pthread_mutex_lock(&mutex_fisiere);
    FILE*ff=fopen(buff,"wb");

    fwrite(down_rs->down_req->file_content,1,down_rs->down_req->nr_octeti_file_content,ff);

    fclose(ff);
    pthread_mutex_unlock(&mutex_fisiere);
    send(down_rs->sock,down_rs->down_req,sizeof(down_req_t),0);

    writeInLogs("0x1",down_rs->down_req->cale_fisier,NULL);
    free(down_rs);


    return NULL;
}

void*handle_upload_operation(void* args) {

    upload_req_start_t*upload_rs=(upload_req_start_t*)args;
    pthread_mutex_lock(&mutex_fisiere);
    FILE*f=fopen(upload_rs->upload_req->cale_fisier,"w");

    fwrite(upload_rs->upload_req->file_content,1,upload_rs->upload_req->nr_octeti_file_content,f);

    fclose(f);
    pthread_mutex_unlock(&mutex_fisiere);
    send(upload_rs->sock,upload_rs->upload_req,sizeof(upload_req_t),0);
    writeInLogs("0x2",upload_rs->upload_req->cale_fisier,NULL);
    return NULL;
}

void* handle_delete_operation(void* args) {
    pid_t pid;
    delete_req_start_t* delete_rs = (delete_req_start_t*)args;

    pid = fork();
    if (pid < 0) {
        delete_rs->delete_req->status = 0x1;  // Eroare de fork
        send(delete_rs->sock, delete_rs->delete_req, sizeof(delete_req_t), 0);
    } else if (pid == 0) {
        char *rm_args[] = {"rm", delete_rs->delete_req->cale_fisier, NULL};
        execvp("rm", rm_args);
        
        perror("Execvp a eșuat");
        exit(1); 
    } else {
        int status;
        waitpid(pid, &status, 0);  

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            delete_rs->delete_req->status = 0x0;  
        } else {
            delete_rs->delete_req->status = 0x2;  // Eroare la execuția rm
        }

        send(delete_rs->sock, delete_rs->delete_req, sizeof(delete_req_t), 0);
    }

    writeInLogs("0x4",delete_rs->delete_req->cale_fisier,NULL);

    free(delete_rs);

    return NULL;
}

void* handle_move_operation(void* args) {
    move_req_start_t* move_rs = (move_req_start_t*)args;
    pthread_t ttid;

    printf("Cale fișier sursă: %s\n", move_rs->move_req->cale_sursa);
    printf("Cale fișier destinație: %s\n", move_rs->move_req->cale_destinatie);

    delete_req_start_t* delete_rs = (delete_req_start_t*)malloc(sizeof(delete_req_start_t));
    delete_rs->delete_req = (delete_req_t*)malloc(sizeof(delete_req_t));

    delete_rs->delete_req->nr_oct_cale_fisier = move_rs->move_req->nr_octeti_cale_sursa;
    strcpy(delete_rs->delete_req->cale_fisier, move_rs->move_req->cale_sursa);

    pthread_mutex_lock(&mutex_fisiere);
    FILE* f = fopen(move_rs->move_req->cale_sursa, "r");
    if (f == NULL) {
        perror("ERR opens file sursa\n");
        move_rs->move_req->status = 0x1;
        send(move_rs->sock, move_rs->move_req, sizeof(move_req_t), 0);  
        return NULL;
    }

    fread(move_rs->move_req->content_sursa, 1, 4096, f);
    fclose(f);
    pthread_mutex_unlock(&mutex_fisiere);

    move_rs->move_req->content_sursa[strlen(move_rs->move_req->content_sursa)] = '\0';
    pthread_create(&ttid, NULL, handle_delete_operation, (void*)delete_rs);
    pthread_join(ttid, NULL);

    pthread_mutex_lock(&mutex_fisiere);
    FILE* f1 = fopen(move_rs->move_req->cale_destinatie, "w");
    if (f1 == NULL) {
        perror("ERR opens file destinatie\n");
        move_rs->move_req->status = 0x1;
        send(move_rs->sock, move_rs->move_req, sizeof(move_req_t), 0);  
        return NULL;
    }

    if (fwrite(move_rs->move_req->content_sursa, 1, strlen(move_rs->move_req->content_sursa), f1) != 0) {
        move_rs->move_req->status = 0x0;  
    } else {
        move_rs->move_req->status = 0x1;  
    }

    fclose(f1);
    pthread_mutex_unlock(&mutex_fisiere);

    send(move_rs->sock, move_rs->move_req, sizeof(move_req_t), 0);

    writeInLogs("0x8",move_rs->move_req->cale_sursa,NULL);

    return NULL;
}

void* handle_update_operation(void* args) {
    update_req_start_t* update_rs = (update_req_start_t*)args;

    pthread_mutex_lock(&mutex_fisiere);
    FILE* f = fopen(update_rs->update_req->cale_fisier, "r+");
    if (f == NULL) {
        perror("ERR open file for update\n");
        update_rs->update_req->status = 0x1;  
        send(update_rs->sock, update_rs->update_req, sizeof(update_req_t), 0);
        pthread_mutex_unlock(&mutex_fisiere);
        return NULL;
    }

    if (fseek(f, update_rs->update_req->octet_start, SEEK_SET) != 0) {
        perror("ERR seek file position\n");
        update_rs->update_req->status = 0x2;  
        send(update_rs->sock, update_rs->update_req, sizeof(update_req_t), 0);
        fclose(f);
        pthread_mutex_unlock(&mutex_fisiere);
        return NULL;
    }

    if (fwrite(update_rs->update_req->new_ch, 1, update_rs->update_req->dim, f) != update_rs->update_req->dim) {
        perror("ERR write to file\n");
        update_rs->update_req->status = 0x3;  
        send(update_rs->sock, update_rs->update_req, sizeof(update_req_t), 0);
        fclose(f);
        pthread_mutex_unlock(&mutex_fisiere);
        return NULL;
    }

    update_rs->update_req->status = 0x0;  
    send(update_rs->sock, update_rs->update_req, sizeof(update_req_t), 0);
    writeInLogs("0x10",update_rs->update_req->cale_fisier,NULL);
    fclose(f);
    pthread_mutex_unlock(&mutex_fisiere);
    return NULL;
}

void* handle_search_operation(void* args) {
    search_req_start_t* search_rs = (search_req_start_t*)args;
    search_req_t* req = search_rs->search_req;
    
    char command[512];
    char buffer[1024];

    pthread_mutex_lock(&mutex_fisiere);
    FILE* fp;

    snprintf(command, sizeof(command), "grep -l -r '%s' %s", req->cuv, ROOT_DIR);

    fp = popen(command, "r");
    if (fp == NULL) {
        perror("Eroare la execuția comenzii grep");
        req->nr_oct_dim_lista = 0;
        send(search_rs->sock, req, sizeof(search_req_t), 0);
        return NULL;
    }

    req->nr_oct_dim_lista = 0;
    memset(req->lista_fisiere, 0, sizeof(req->lista_fisiere));
    
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        
        strcat(req->lista_fisiere, buffer);
        strcat(req->lista_fisiere, "\n");  
        req->nr_oct_dim_lista += strlen(buffer) + 1;
    }

    pclose(fp);
    pthread_mutex_unlock(&mutex_fisiere);
    send(search_rs->sock, req, sizeof(search_req_t), 0);
    writeInLogs("0x20",NULL,search_rs->search_req->cuv);
    free(search_rs);
    return NULL;
}

void* handle_client(void* arg) {
    int sock = *(int*)arg;

    client_info_t *client_info = (client_info_t*)malloc(sizeof(client_info_t));
    client_info->client_socket = sock;

    int client_socket = client_info->client_socket;
    uint32_t op_code_recv;
    int bytes_received;

    sem_wait(&sem_clienti);

    while (1) {
        bytes_received = recv(client_socket, &op_code_recv, sizeof(op_code_recv), 0);
        if (bytes_received <= 0) {
            printf("Clientul s-a deconectat\n");
            break;  
        }

        printf("Mesaj de la client: %d\n", op_code_recv);

        switch (op_code_recv) {
            case 0x0:
                printf("Operație: LIST\n");
                list_req_start_t* list_rs=(list_req_start_t*)malloc(sizeof(list_req_start_t));
                list_rs->sock=client_info->client_socket;
                pthread_create(&client_info->tid,NULL,handle_list_operation,(void*)list_rs);
                pthread_join(client_info->tid,NULL);
                break;
            case 0x1: 
                printf("Operație: DOWNLOAD\n");

                down_req_start_t* down_rs = (down_req_start_t*)malloc(sizeof(down_req_start_t));
                down_rs->sock = client_info->client_socket;
                down_rs->down_req = (down_req_t*)malloc(sizeof(down_req_t));

                recv(down_rs->sock, down_rs->down_req, sizeof(down_req_t), 0);

                printf("Calea fișierului si oct cale: %s %d\n", down_rs->down_req->cale_fisier,down_rs->down_req->nr_octeti_cale_fisier);

                pthread_create(&client_info->tid, NULL, handle_download_operation, (void*)down_rs);
                
                pthread_join(client_info->tid, NULL);
                
                break;

            case 0x2:
                printf("Operație: UPLOAD\n");
                upload_req_start_t* upload_rs=(upload_req_start_t*)malloc(sizeof(upload_req_start_t));
                upload_rs->sock=client_info->client_socket;
                upload_rs->upload_req=(upload_req_t*)malloc(sizeof(upload_req_t));
                recv(upload_rs->sock,upload_rs->upload_req,sizeof(upload_req_t),0);

                pthread_create(&client_info->tid,NULL,handle_upload_operation,(void*)upload_rs);
                pthread_join(client_info->tid,NULL);
                break;
            case 0x4:
                printf("Operație: DELETE\n");
                delete_req_start_t*delete_rs=(delete_req_start_t*)malloc(sizeof(delete_req_start_t));
                delete_rs->sock=client_info->client_socket;
                delete_rs->delete_req=(delete_req_t*)malloc(sizeof(delete_req_t));
                recv(delete_rs->sock,delete_rs->delete_req,sizeof(delete_req_t),0);

                pthread_create(&client_info->tid,NULL,handle_delete_operation,(void*)delete_rs);
                pthread_join(client_info->tid,NULL);
                break;
            case 0x8:
                printf("Operație: MOVE\n");
                move_req_start_t*move_rs=(move_req_start_t*)malloc(sizeof(move_req_start_t));
                move_rs->sock=client_info->client_socket;
                move_rs->move_req=(move_req_t*)malloc(sizeof(move_req_t));
                recv(move_rs->sock,move_rs->move_req,sizeof(move_req_t),0);
                printf("Cale fișier sursă: %s\n", move_rs->move_req->cale_sursa);
                printf("Cale fișier destinație: %s\n", move_rs->move_req->cale_destinatie);

                pthread_create(&client_info->tid,NULL,handle_move_operation,(void*)move_rs);
                pthread_join(client_info->tid,NULL);

                break;
            case 0x10:
                printf("Operație: UPDATE\n");
                update_req_start_t*update_rs=(update_req_start_t*)malloc(sizeof(update_req_start_t));
                update_rs->sock=client_info->client_socket;
                update_rs->update_req=(update_req_t*)malloc(sizeof(update_req_t));

                recv(update_rs->sock,update_rs->update_req,sizeof(update_req_t),0);

                pthread_create(&client_info->tid,NULL,handle_update_operation,(void*)update_rs);
                pthread_join(client_info->tid,NULL);

                break;
            case 0x20:
                printf("Operație: SEARCH\n");
                search_req_start_t*search_rs=(search_req_start_t*)malloc(sizeof(search_req_start_t));
                search_rs->sock=client_info->client_socket;
                search_rs->search_req=(search_req_t*)malloc(sizeof(search_req_t));

                recv(search_rs->sock,search_rs->search_req,sizeof(search_req_t),0);
                pthread_create(&client_info->tid,NULL,handle_search_operation,(void*)search_rs);
                pthread_join(client_info->tid,NULL);
                break;
            default:
                printf("Operație necunoscută\n");
                break;
        }

        send(client_socket, &op_code_recv, sizeof(op_code_recv), 0);
    }
    sem_post(&sem_clienti);

    close(client_socket);
    free(client_info);
    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    pthread_t client_threads[MAX_CLIENTS];
    pthread_t signal_thread;

    int client_count = 0;

    int N = 5;  
    sem_init(&sem_clienti, 0, N);


    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Nu s-a putut crea socketul serverului");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080); 
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Nu s-a putut lega socketul de port");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Nu s-a putut seta serverul pentru ascultare");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Serverul a pornit și ascultă pe portul 8080...\n");

    pthread_create(&signal_thread, NULL, handle_signals, NULL);

    while (!stop_server) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            if (stop_server) break;  
            perror("Eroare la acceptarea conexiunii");
            continue;
        }

        if (pthread_create(&client_threads[client_count], NULL, handle_client, (void*)&client_socket) != 0) {
            perror("Nu s-a putut crea thread-ul pentru client");
            close(client_socket);
            continue;
        }

        pthread_detach(client_threads[client_count]);

        client_count++;
        if (client_count >= MAX_CLIENTS) {
            client_count = 0; 
        }

        printf("Client conectat\n");
    }

    for (int i = 0; i < client_count; i++) {
        pthread_join(client_threads[i], NULL);
    }

    close(server_socket);
    return 0;
}