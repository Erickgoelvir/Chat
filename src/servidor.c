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

    //Abrir usuarios.txt (aun viendo si lo necesito)

    //Crear los mutex candados
    // pthread_mutex_t bloquear_txt = PTHREAD_MUTEX_INITIALIZER;
    // pthread_mutex_t bloquear_bin = PTHREAD_MUTEX_INITIALIZER;
    // pthread_mutex_t bloquear_fifo = PTHREAD_MUTEX_INITIALIZER;
    //Los declare en funciones_servidor, si funciona eliminar este bloque


    //Crear PIPE
    int fd[2];
    if (pipe(fd) == -1) {perror("Error pipe\n"); return 1;}

    //Fork()
    pid_t pid = fork();
    if (pid == -1) {perror("Error fork"); return 1;}

    //Proceso Hijo
    if (pid == 0) {

        close(fd[0]);

        //Crear y abri FIFO de login (común para todos los clientes)
        if (mkfifo("../data/login/login_requests", 0666) == -1) {perror("Error makefifo\n"); return 1;}
        int login_request = open("../data/login/login_requests", O_RDONLY);
        if (login_request == -1) {perror("Error al abrir login_requests\n"); return 1;}

        //Hilos
        int n = 100;
        pthread_t hilos[n];
        int ids[n];
        int i = 0;

        //SEGUIR CON EL PROCESO DE LOGIN LA INFO ESTA EN EL DIAGRAMA, HAY FUNCIONES UTILES PARA USAR AQUÍ EN EL ARCHIVO DE FUNCIONES_SERVIDOR
        // while (detener) {
        //     datos_login datos_login;
        //     read(login_request, &datos_login, sizeof(datos_login));
        //
        //     pthread_create(&hilos[i], NULL, &datos_login, &datos_login);
        //
        //     i++;
        // }

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
    detener = 0;

    //Esperando a los hilos
    for (int j = 0; j < i; j++) {
        pthread_join(hilos[j], NULL);
    }

    //Mensaje de salida a todos los usuarios
    mensaje_salida();

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

    //Agregar salida al chat.log
    char log_salida[50];
    snprintf(log_salida, sizeof(log_salida), "[SERVIDOR] OFF | %s\n", obtener_fecha());
    escribir_txt("../data/chat.log", log_salida);

    printf("BYE\n");

    return 0;
}




