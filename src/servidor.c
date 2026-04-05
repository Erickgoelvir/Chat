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

//MANEJAR EL SIGINT Y APAGADO DEL SERVIDOR
volatile sig_atomic_t running = 1;
void manejar_sigint(int sig) {
    running = 0;
}

int main(void) {
    //Inicializar SIGINT para salir con ctrl+c
    struct sigaction sa;
    sa.sa_handler = manejar_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);

    //Marcar inicio del servidor
    actualizar_estado(1);

    //inicializar usuarios.bin
    inicializar_archivo_bin();

    //Agregar log a chat.log
    char mensaje[100];
    snprintf(mensaje, sizeof(mensaje), "[SERVIDOR] ON | %s", obtener_fecha());
    escribir_txt("../data/chat.log", mensaje);

    //Servidor Encendido
    printf("INICIANDO SERVIDOR\n");
    printf("[SERVER] ON\n");

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

        datos_login datos_login;

        while (running) {

            ssize_t n = read(login_request, &datos_login, sizeof(datos_login));

            if (n <= 0) {continue;}

            //Fifo para enviar respuesta
            char ruta_resp[50];
            snprintf(ruta_resp, sizeof(ruta_resp), "../data/login/confirmacion%d", datos_login.pid);

            int fd_res = open(ruta_resp, O_WRONLY);
            if (fd_res == -1) {
                perror("Error al abrir FIFO de respuesta\n");
                continue;
            }

            int resp = ver_existencia_usuario(datos_login.nombre);
            write(fd_res, &resp, sizeof(resp));

            if (resp == 0) {
                char ruta_outbox[100], ruta_inbox[100];
                snprintf(ruta_outbox, sizeof(ruta_outbox), "../data/outbox/cliente%d", datos_login.pid);
                snprintf(ruta_inbox,  sizeof(ruta_inbox),  "../data/inbox/cliente%d",  datos_login.pid);
                if (mkfifo(ruta_outbox, 0666) == -1) {perror("Error mkfifo outbox cliente\n"); return 1;}
                if (mkfifo(ruta_inbox, 0666) == -1) {perror("Error mkfifo inbox cliente\n"); return 1;}
                escribir_bin(datos_login);
                char log[50];
                snprintf(log, sizeof(log), "%s | %s\n", datos_login.nombre, obtener_fecha());
                escribir_txt("../data/usuarios.txt", log);
                escribir_txt("../data/chat.log", log);
                write(fd[1], &datos_login, sizeof(datos_login));
            }
            close(fd_res);
            //unlink(ruta_resp);

        }

        close(fd[1]);
        close(login_request);
        unlink("../data/login/login_requests");

    } //Fin Proceso Hijo, Login

    else {
        close(fd[1]);
        datos_login datos_usuario;

        while (running) {
            ssize_t retorno_pipe = read(fd[0], &datos_usuario, sizeof(datos_usuario));
            if (retorno_pipe == (ssize_t)sizeof(datos_usuario)) {

                datos_login *cliente = malloc(sizeof(datos_login));
                if (!cliente) {perror("Malloc"); continue;}
                *cliente = datos_usuario;

                if (i < 100) {
                    pthread_create(&hilos[i], NULL, atender_cliente, (void*)cliente);
                    i++;
                } else {
                    printf("[SERVER limite de clientes alcanzado]");
                    free(cliente);
                }

            } else if (retorno_pipe == -1) {
                perror("Error al abrir login_requests\n"); return 1;
            }
            i++;
        }
        close (fd[0]);

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
    }

    return 0;
}




