//
// Created by erick on 3/28/26.
//
#include <stdio.h>
#include <signal.h>
#include "./headers/servidor.h"
#include <stdlib.h>
#include "./headers/funciones_servidor.h"
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>


int main(void) {

    //Marcar inicio del servidor
    actualizar_estado(1);

    //inicializar usuarios.bin
    inicializar_archivo_bin();

    //Agregar log a chat.log
    char mensaje[50];
    snprintf(mensaje, sizeof(mensaje), "[SERVIDOR] ON | %s", obtener_fecha());
    escribir_txt("../data/chat.log", mensaje);

    //Servidor Encendido
    printf("INICIANDO SERVIDOR\n");
    printf("[SEVER] ON\n");

    //Inicializar SIGINT para salir con ctrl+c
    signal(SIGINT, manejar_sigint);

    //iniciar variable Detener
    //int detener = 1;

    //Bajo ninguna circunstancia tocar esta variable, hace cosas importantes
    int i = 0;
    pthread_t hilos[100];
    int id[100];

    //Crear PIPE
    int fd[2];
    if (pipe(fd) == -1) {perror("Error pipe\n"); return 1;}

    //Fork()
    pid_t pid = fork();
    if (pid == -1) {perror("Error fork"); return 1;}

    //Proceso Hijo
    if (pid == 0) {

        close(fd[0]);

        //Asegurarse de que el FIFO no exista antes
        unlink("../data/login/login_requests");

        //Crear y abri FIFO de login (común para todos los clientes)
        if (mkfifo("../data/login/login_requests", 0666) == -1) {perror("Error makefifo\n"); return 1;}
        int login_request = open("../data/login/login_requests", O_RDONLY  /*| O_NONBLOCK*/);
        if (login_request == -1) {perror("Error al abrir login_requests\n"); return 1;}

        printf("[PORTERO] Esperando conexiones...\n");

        //Hilos
        pthread_t hilos_login[100];
        int ids[100];
        int j = 0; //control de hilos creados por login

        //SEGUIR CON EL PROCESO DE LOGIN LA INFO ESTA EN EL DIAGRAMA, HAY FUNCIONES UTILES PARA USAR AQUÍ EN EL ARCHIVO DE FUNCIONES_SERVIDOR
        // while (running) {
        //     datos_login datos_login;
        //     read(login_request, &datos_login, sizeof(datos_login));
        //
        //     pthread_create(&hilos[i], NULL, &datos_login, &datos_login);
        //
        //     i++;
        // }

        close(login_request);
        unlink("../data/login/login_requests");

    } //Fin Proceso Hijo, Login

    else {

        close(fd[1]);
        datos_login datos_usuario;

        while (running) {

            if (read(fd[0], &datos_usuario, sizeof(datos_usuario)) > 0) {

                pthread_create(&hilos[i], NULL, atender_cliente, (void*)&datos_usuario);

            } else {perror("Error al abrir login_requests\n"); return 1;}
            i++;
        }
        close (fd[0]);

    }

    //Detener los hilos que atienden a los clientes, y el subproceso login
    detener_hilos();

    //Mensaje de salida a todos los usuarios
    mensaje_salida();

    //Esperando a los hilos
    for (int j = 0; j < i; j++) {
        pthread_join(hilos[j], NULL);
    }

    //esperar que se reciba el mensaje
    sleep(3);

    //Eliminar los archivos de los usuarios
    eliminar_fifos();

    //Esperando al proceso hijo
    waitpid(pid, NULL, 0);

    //Destruir mutex
    pthread_mutex_destroy(&bloquear_txt);
    pthread_mutex_destroy(&bloquear_bin);
    pthread_mutex_destroy(&bloquear_fifo);

    //Actualizar estado
    actualizar_estado(0);

    //Limpiar archivos de registro de usuarios
    FILE *usuarios_bin = fopen("../data/usuarios.bin", "wb");
    FILE *usuarios_txt = fopen("../data/usuarios.txt", "w");
    if (usuarios_bin == NULL || usuarios_txt == NULL) {
        perror("Error al abrir usuarios\n");
    } else {
        fclose(usuarios_bin);
        fclose(usuarios_txt);
    }

    //Agregar salida al chat.log
    char log_salida[50];
    snprintf(log_salida, sizeof(log_salida), "[SERVIDOR] OFF | %s\n", obtener_fecha());
    escribir_txt("../data/chat.log", log_salida);

    printf("BYE\n");

    return 0;
}




