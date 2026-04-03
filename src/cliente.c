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
        return 1;
    }

    printf("BIENVENIDO\n");
    printf("Nombre: \n");
    //seguir

    return 0;

}

int is_server_run() {

    //IMPORTANTE: prueba que el proceso servidor está en ejecución

    FILE *status = fopen("../data/server_status.log", "rb");
    if (status == NULL) {perror("error al abrir"); return 1;}
    int buffer;
    fread(&buffer, sizeof(int), sizeof(buffer), status);
    fclose(status);
    return buffer;

}