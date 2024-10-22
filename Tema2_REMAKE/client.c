#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

typedef struct{

    int nr_octeti_cuv;
    char cuv[25];
    int nr_oct_dim_lista;
    char lista_fisiere[4096];

}search_req_t;
typedef struct{

    int status;
    int nr_oct_cale_fisier;
    char cale_fisier[30];
    int octet_start;
    int dim;
    char new_ch[4096];

}update_req_t;

typedef struct{

    int status;
    int nr_octeti_cale_sursa;
    int nr_octeti_cale_destinatie;
    char content_sursa[4096];
    char cale_sursa[30];
    char cale_destinatie[30];

}move_req_t;

typedef struct{

    uint32_t status;
    int nr_oct_cale_fisier;
    char cale_fisier[30];

}delete_req_t;

typedef struct{

    uint32_t status;
    int nr_oct_cale_fisier;
    int nr_octeti_file_content;
    char cale_fisier[30];
    char file_content[4096];

}upload_req_t;

// typedef struct {
//     char cale_fisier[30];
//     int nr_octeti_cale_fisier;
// } down_req_info_t;  // Pentru cererea de download

// typedef struct {
//     uint32_t status;
//     char file_content[4096];
//     int nr_octeti_file_content;
// } down_resp_t;  // Pentru răspunsul de la server (conținutul fișierului)


typedef struct{
    uint32_t status;
    char file_content[4096];
    int nr_octeti_file_content;
    int nr_octeti_cale_fisier;
    char cale_fisier[30];
}down_req_t;

typedef struct{
    uint32_t status;
    int nr_octeti_rsapuns;
    char file_list[5000];
}list_req_t;


void print_menu() {
    printf("Comenzi disponibile:\n");
    printf("0x0 - LIST\n");
    printf("0x1 - DOWNLOAD\n");
    printf("0x2 - UPLOAD\n");
    printf("0x4 - DELETE\n");
    printf("0x8 - MOVE\n");
    printf("0x10 - UPDATE\n");
    printf("0x20 - SEARCH\n");
}

int main()
{
    int sock;
    struct sockaddr_in server_addr;
    uint32_t op_code;
    uint32_t server_response;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Nu s-a putut crea socketul clientului");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);  
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Nu s-a putut conecta la server");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Conectat la server. Trimite mesaje (scrie 'exit' pentru a ieși):\n");

    while (1) {
        print_menu();
        
        scanf("%x", &op_code);

        send(sock, &op_code, sizeof(op_code), 0);

        if (op_code == 0x0) {  
            printf("\n\nOperație LIST selectată.\n");


            list_req_t* list_resp = (list_req_t*)malloc(sizeof(list_req_t));

            recv(sock, list_resp, sizeof(list_req_t), 0);

            if (list_resp->status == 0x0) {
                printf("Fișiere de pe server:\n%s\n", list_resp->file_list);
                printf("Număr octeți output: %d\n", list_resp->nr_octeti_rsapuns);
            } else {
                printf("Eroare la listarea fișierelor: cod %x\n", list_resp->status);
            }

            printf("Răspuns de la server: 0x%x\n", list_resp->status);

            free(list_resp);  
            
        }
       
        else if (op_code == 0x1) {
            printf("Operație DOWNLOAD selectată.\n");
            down_req_t* down_req=(down_req_t*)malloc(sizeof(down_req_t));
            printf("Introduceti nr octeti cale fisier (de exemplu: 22 pentru './files/files1/1.txt'):\n");
            scanf("%d", &down_req->nr_octeti_cale_fisier);
            printf("Introduceti cale fisier (de exemplu: './files/files1/1.txt'):\n");
            scanf("%s", down_req->cale_fisier);
            printf("%d %s\n",down_req->nr_octeti_cale_fisier,down_req->cale_fisier);
            send(sock, down_req, sizeof(down_req_t), 0);

            down_req_t* down_resp=(down_req_t*)malloc(sizeof(down_req_t));

            recv(sock,down_resp,sizeof(down_req_t),0);
            if(down_resp->file_content!=NULL&&down_resp->nr_octeti_file_content!=0)down_resp->status=0x0;
            else down_resp->status=0x1;
            printf("%d:%s\nstatus:%x\n\n",down_resp->nr_octeti_file_content,down_resp->file_content,down_resp->status);
        }

        else if (op_code == 0x2) {
            printf("Operație UPLOAD selectată.\n");
            upload_req_t*upload_req=(upload_req_t*)malloc(sizeof(upload_req_t));
            printf("octeti cale fisier:");
            scanf("%d",&upload_req->nr_oct_cale_fisier);
            printf("cale fisier:");
            scanf("%s",upload_req->cale_fisier);
            printf("octeti continut fisier:");
            scanf("%d",&upload_req->nr_octeti_file_content);
            printf("continut fisier:");
            scanf("%s",upload_req->file_content);

            send(sock,upload_req,sizeof(upload_req_t),0);

            upload_req_t*after_recv=(upload_req_t*)malloc(sizeof(upload_req_t));

            recv(sock,after_recv,sizeof(upload_req_t),0);

            FILE*f=fopen(after_recv->cale_fisier,"r");
            if(f==NULL)after_recv->status=0x1;
            else after_recv->status=0x0;
            fclose(f);

            printf("STATUS:%x\n",after_recv->status);

            printf("\n\n");
        }

        else if (op_code == 0x4) {
            printf("Operație DELETE selectată.\n");
            
            delete_req_t* delete_req=(delete_req_t*)malloc(sizeof(delete_req_t));
            printf("octeti cale fisier:");
            scanf("%d",&delete_req->nr_oct_cale_fisier);
            printf("cale fisier:");
            scanf("%s",delete_req->cale_fisier);

            send(sock,delete_req,sizeof(delete_req_t),0);

            delete_req_t* after_recv=(delete_req_t*)malloc(sizeof(delete_req_t));
            
            recv(sock,after_recv,sizeof(delete_req_t),0);
            printf("%x\n\n",after_recv->status);
            printf("\n\n");
        }

        else if (op_code == 0x8) {
            printf("Operație MOVE selectată.\n");
            move_req_t*move_req=(move_req_t*)malloc(sizeof(move_req_t));
            printf("octeti cale fisier sursa:");
            scanf("%d",&move_req->nr_octeti_cale_sursa);
            printf("cale fisier sursa:");
            scanf("%s",move_req->cale_sursa);
            printf("octeti cale fisier destinatie:");
            scanf("%d",&move_req->nr_octeti_cale_destinatie);
            printf("cale fisier destinatie:");
            scanf("%s",move_req->cale_destinatie);
            move_req->cale_sursa[strlen(move_req->cale_sursa)]='\0';
            move_req->cale_destinatie[strlen(move_req->cale_destinatie)]='\0';
            printf("Cale fișier sursă: %s\n", move_req->cale_sursa);
            printf("Cale fișier destinație: %s\n", move_req->cale_destinatie);
            send(sock,move_req,sizeof(move_req_t),0);

            move_req_t* after_recv=(move_req_t*)malloc(sizeof(move_req_t));
            
            recv(sock,after_recv,sizeof(move_req_t),0);
            printf("%x\n\n",after_recv->status);

            printf("\n\n");
        }

        else if (op_code == 0x10) {
            printf("Operație UPDATE selectată.\n");
            update_req_t*update_req=(update_req_t*)malloc(sizeof(update_req_t));
            printf("octeti cale:");
            scanf("%d",&update_req->nr_oct_cale_fisier);
            printf("cale fisier:");
            scanf("%s",update_req->cale_fisier);
            printf("octet start si dimensiune:");
            scanf("%d %d",&update_req->octet_start,&update_req->dim);
            printf("caractere noi:");
            scanf("%s",update_req->new_ch);
            update_req->new_ch[strlen(update_req->new_ch)]='\0';
            send(sock,update_req,sizeof(update_req_t),0);

            update_req_t* after_recv = (update_req_t*)malloc(sizeof(update_req_t));
            recv(sock, after_recv, sizeof(update_req_t), 0);

            if (after_recv->status == 0x0) {
                printf("UPDATE reușit.\n");
            } else {
                printf("UPDATE eșuat, cod eroare: %x\n", after_recv->status);
            }

            free(update_req);
            free(after_recv);
            printf("\n\n");
        }

        else if (op_code == 0x20) {
            printf("Operație SEARCH selectată.\n");
            search_req_t*search_req=(search_req_t*)malloc(sizeof(search_req_t));
            printf("octeti cuvant:");
            scanf("%d",&search_req->nr_octeti_cuv);
            printf("cuvant:");
            scanf("%s",search_req->cuv);
            search_req->cuv[strlen(search_req->cuv)]='\0';
            send(sock,search_req,sizeof(search_req_t),0);

            search_req_t* search_resp = (search_req_t*)malloc(sizeof(search_req_t));
            recv(sock, search_resp, sizeof(search_req_t), 0);

            if (search_resp->nr_oct_dim_lista > 0) {
                printf("Fișierele care conțin cuvântul '%s':\n", search_req->cuv);

                char* file = search_resp->lista_fisiere;
                while (*file) {
                    printf("%s\n", file);
                    file += strlen(file) + 1;
                }
            } else {
                printf("Cuvântul '%s' nu a fost găsit în niciun fișier.\n", search_req->cuv);
            }

            free(search_req);
            free(search_resp);
            printf("\n\n");
        }

        int bytes_received = recv(sock, &server_response, sizeof(server_response), 0);
        if (bytes_received <= 0) {
            printf("Conexiunea cu serverul a fost închisă.\n");
            break;
        }

    }

    close(sock);
    return 0;
}
