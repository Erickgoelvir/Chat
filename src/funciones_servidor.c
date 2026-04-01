//
// Created by erick on 3/28/26.
//
#include "headers/funciones_servidor.h"

#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

pthread_mutex_t bloquear_txt = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bloquear_bin = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bloquear_fifo = PTHREAD_MUTEX_INITIALIZER;

//MANEJAR EL SIGINT Y APAGADO DEL SERVIDOR
volatile sig_atomic_t running = 1;
void manejar_sigint(int sig) {
    running = 0;
}

//ACTUALIZAR EL ESTADO [ON/OFF] DEL SERVIDOR
void actualizar_estado(int estado) {

    FILE *status = fopen("../data/server_status.log", "wb");
    if (status == NULL) {perror("error al actualizar el estado\n");}
    fwrite(&estado, sizeof(int), 1, status);
    fclose(status);

}

//FUNCIONES PARA ABRIR LOS ARCHIVOS CON MUTEX

void escribir_txt(char *direccion, char *mensaje) {
    pthread_mutex_lock(&bloquear_txt);

    FILE *archivo= fopen(direccion,"a");
    if (archivo == NULL) {
        perror("error al abrir el archivo\n");
    } else {
        fprintf(archivo,"%s\n", mensaje);
        fclose(archivo);
    }

    pthread_mutex_unlock(&bloquear_txt);
}

void inicializar_archivo_bin() {
    FILE *archivo = fopen("../data/usuarios.bin", "wb");
    if (archivo) {
        datos_login vacio = {0, ""};
        for (int i = 0; i < 100; i++) {
            fwrite(&vacio, sizeof(datos_login), 1, archivo);
        }
        fclose(archivo);
    }
}

void escribir_bin(const datos_login datos) {
    pthread_mutex_lock(&bloquear_bin);
    FILE *archivo = fopen("../data/usuarios.bin", "ab");
    if (archivo != NULL) {
        fwrite(&datos, sizeof(datos_login), 1, archivo);
        fclose(archivo);
    }
    pthread_mutex_unlock(&bloquear_bin);
}

void eliminar_usuario_txt (const char *nombre_borrar) {
    pthread_mutex_lock(&bloquear_txt);

    FILE *archivo = fopen("../data/usuarios.txt", "r");
    FILE *temp = fopen("../data/usuarios_temp.txt", "w");
    if (!archivo || !temp) {
        perror("Error al abrir el archivo\n");
        if (archivo) fclose(archivo);
        if (temp) fclose(temp);
        pthread_mutex_unlock(&bloquear_txt);
        return;
    }

    char linea[100], nombre[20];
    int encontrado = 0;

    while (fgets(linea, sizeof(linea), archivo)) {
        sscanf(linea, " %[^| ]", nombre);
        if (!strcmp(nombre, nombre_borrar)) {
            encontrado = 1;
            continue;
        }
        fputs(linea, temp);
    }

    fclose(archivo);
    fclose(temp);

    if (encontrado) {
        remove("../data/usuarios.txt");
        rename("../data/usuarios_temp.txt", "../data/usuarios.txt");
    } else {
        remove("../data/usuarios_temp.txt");
    }

    pthread_mutex_unlock(&bloquear_txt);
}

void eliminar_usuario_bin(const char *nombre_borrar) {
    pthread_mutex_lock(&bloquear_bin);

    FILE *archivo = fopen("../data/usuarios.bin", "r+b");
    if (!archivo) {
        pthread_mutex_unlock(&bloquear_bin);
        return;
    }
    datos_login registro, vacio = {0, ""};

    while (fread(&registro, sizeof(registro), 1, archivo)) {
        if (!strcmp(registro.nombre, nombre_borrar)) {
            fseek(archivo, -sizeof(registro), SEEK_CUR);
            fwrite(&vacio, sizeof(vacio), 1, archivo);
            break;
        }
    }
    fclose(archivo);

    pthread_mutex_unlock(&bloquear_bin);
}

void escribir_fifo(const int pid, const char mensaje) {
    pthread_mutex_lock(&bloquear_fifo);
    char direccion[50];
    snprintf(direccion, sizeof(direccion), "../data/inbox/cliente%d", pid);

    int fd = open(direccion, O_WRONLY);
    if (fd == -1) {
        perror("Error al abrir archivo\n");
    } else {
        write(fd, mensaje, strlen(mensaje));
    }

    close(fd);

    pthread_mutex_unlock(&bloquear_fifo);
}

//Function controladora de los hilos que atienden al usuario
void *atender_cliente(void *arg) {
    datos_login* cliente = (datos_login*)arg;
    char direccion_outbox[50];
    snprintf(direccion_outbox, sizeof(direccion_outbox), "../data/outbox/cliente%d", cliente->pid);
    int fd = open(direccion_outbox, O_RDONLY);
    if (fd == -1) {perror("Error al abrir archivo\n"); return NULL;}
    datos_operacion operacion;

    FILE *usuarios = fopen("../data/usuarios.bin", "rb");
    if (!usuarios) {perror("Error al abrir archivo de usuarios\n"); return NULL;}

    while (detener) {
        ssize_t n = read(fd, &operacion, sizeof(datos_operacion));
        if (n > 0) {

            if (strcmp(operacion.comando, "usuarios") == 0) {

                datos_login registro;
                char buffer[1024];
                buffer[0] = '\0';

                while (fread(&registro, sizeof(datos_login), 1, usuarios) == 1) {
                    strcat(buffer, registro.nombre);
                    strcat(buffer, "\n");
                }
                escribir_fifo(cliente->pid, buffer);

            } else if (strcmp(operacion.comando, "privado") == 0) {

                datos_login registro;
                pid_t pid_encontrado = -1;

                while (fread(&registro, sizeof(datos_login), 1, usuarios) == 1) {
                    registro.nombre[sizeof(registro.nombre) - 1] = '\0';

                    if (strcmp(registro.nombre, operacion.receptor) == 0) {
                        pid_encontrado = registro.pid;
                        break;
                    }

                }

                char mensaje[100];
                snprintf(mensaje, sizeof(mensaje), "[%s] %s\n", operacion.emisor, operacion.mensaje);

                if (pid_encontrado == -1) {
                    printf("No se envio el mensaje de %s a %s. Nadie se va a dar cuenta\n", operacion.emisor, operacion.receptor);
                } else {
                    escribir_fifo(pid_encontrado, mensaje);
                    escribir_txt("../data/chat.log", mensaje);
                }

            } else if (strcmp(operacion.comando, "global") == 0) {

                datos_login registro;
                char mensaje[100];
                snprintf(mensaje, sizeof(mensaje), "[%s] %s\n", operacion.emisor, operacion.mensaje);

                while (fread(&registro, sizeof(datos_login), 1, usuarios) == 1) {
                    escribir_fifo(registro.pid, mensaje);
                }
                escribir_txt("../data/chat.log", mensaje);

            } else if (strcmp(operacion.comando, "salir") == 0) {

                eliminar_usuario_bin(cliente->nombre);
                eliminar_usuario_txt(cliente->nombre);
                char mensaje[100];
                snprintf(mensaje, sizeof(mensaje), "[%s] cerro sesión | %s\n", cliente->nombre, obtener_fecha());
                escribir_txt("../data/chat.log", mensaje);
                close(fd);
                pthread_exit(NULL);

            } else {
                perror("Error al recibir el comando del cliente\n");
            }

        } else {
            perror("Error al leer el FIFO\n");
            return NULL;
        }
    }
    fclose(usuarios);

    return NULL;
}

//ENVIAR MENSAJE DE APAGADO A LOS CLIENTES
void mensaje_salida() {

    datos_login registro;
    char mensaje[100];
    snprintf(mensaje, sizeof(mensaje), "[Servidor] Apagando el sistema\n");
    escribir_txt("../data/chat.log", mensaje);

    FILE * usuarios = fopen("../data/usuarios.bin", "rb");
    while (fread(&registro, sizeof(datos_login), 1, usuarios) == 1) {
        escribir_fifo(registro.pid, mensaje);
    }

    fclose(usuarios);
}

//Eliminar los inbox y outbox
void eliminar_fifos() {

    datos_login registro;
    char path_inbox[50];
    char path_outbox[50];

    FILE * usuarios = fopen("../data/usuarios.bin", "rb");
    while (fread(&registro, sizeof(datos_login), 1, usuarios) == 1) {
        snprintf(path_inbox, sizeof(path_inbox), "../data/outbox/cliente%d", registro.pid);
        snprintf(path_inbox, sizeof(path_inbox), "../data/inbox/cliente%d", registro.pid);
        if (unlink(path_inbox) == 0 && unlink(path_outbox) == -1) {
            perror("Error al eliminar archivos\n");
        } else {
            printf("Archivos eliminados");
        }
    }

    fclose(usuarios);

}

//FUNCTION PARA DEVOLVER FECHA Y HORA (Para los logs)
char* obtener_fecha() {

    time_t ahora = time(NULL);
    struct tm * t = localtime(&ahora);

    static char fecha[50];
    strftime(fecha, sizeof(fecha), "%d-%m-%y_%H:%M:%S", t);
    return fecha;

}

