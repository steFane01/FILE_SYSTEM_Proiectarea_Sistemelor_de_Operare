#include <stdio.h>
#include "operations.h"

void handle_list(int sock) {
    // Trimitere lista de fișiere către client
    printf("Handling LIST operation\n");
}

void handle_download(int sock) {
    // Trimitere fișier către client
    printf("Handling DOWNLOAD operation\n");
}

void handle_upload(int sock) {
    // Primire și stocare fișier de la client
    printf("Handling UPLOAD operation\n");
}

void handle_delete(int sock) {
    // Ștergere fișier pe server
    printf("Handling DELETE operation\n");
}

void handle_move(int sock) {
    // Mutare fișier pe server
    printf("Handling MOVE operation\n");
}

void handle_update(int sock) {
    // Actualizare conținut fișier
    printf("Handling UPDATE operation\n");
}

void handle_search(int sock) {
    // Căutare cuvânt în fișiere și returnare rezultate
    printf("Handling SEARCH operation\n");
}
