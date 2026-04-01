//
// Created by erick on 3/28/26.
//
#include <stdio.h>
#include "headers/cliente.h"

int is_server_run();

int main(void) {

    int estado_servidor = is_server_run();
    if (estado_servidor != 1) {
        printf("SERVIDOR NO INICIADO\n");
        return 0;
    }

    printf("BIENVENIDO\n");
    printf("Nombre: \n");
    //seguir

    return 0;

}

int is_server_run() {

    //IMPORTANTE: prueba que el proceso servidor esta en ejecucion

    FILE *status = fopen("../data/server_status.log", "rb");
    if (status == NULL) {perror("error al abrir"); return 1;}
    int buffer;
    fread(&buffer, sizeof(char), sizeof(buffer)-1, status);
    fclose(status);
    return buffer;

}