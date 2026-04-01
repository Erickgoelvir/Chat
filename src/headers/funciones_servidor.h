//
// Created by erick on 3/28/26.
//

#ifndef FUNCIONES_SERVIDOR_H
#define FUNCIONES_SERVIDOR_H
#include <signal.h>
#include <pthread.h>

//Struct para recibir información del login
typedef struct {
    pid_t pid;
    char nombre[20];
} datos_login;

//Struct para los datos de la operacion a realizar
typedef struct {
    char comando[20];
    char emisor[20];
    char receptor[20];
    char mensaje[50];
} datos_operacion;

//Manejar el SIGINT
extern volatile sig_atomic_t running;
void manejar_sigint(int sig);

//Actualizar estado del servidor
void actualizar_estado(int);

//Variable para detener los hilos y proceso hijo
int detener = 1;


//Function obtener fecha y hora
char* obtener_fecha();

//Variables Globales MUTEX, Funciones que las usan
pthread_mutex_t bloquear_txt;
pthread_mutex_t bloquear_bin;
pthread_mutex_t bloquear_fifo;

void inicializar_archivo_bin();
void escribir_txt(char *direccion, char *mensaje);
void escribir_bin(datos_login datos);
void eliminar_usuario_txt (const char *nombre_borrar);
void eliminar_usuario_bin(const char *nombre_borrar);
void escribir_fifo(int pid, char *mensaje);

//Function controladora de los hilos que atienden al usuario
void* atender_cliente(void *arg);

//numero de hilos creados para atender a usuarios
int i = 0;
int n = 100;
pthread_t hilos[n];
int id[n];

//Mensaje de salida
void mensaje_salida();

//Eliminar los archivos al salir
void eliminar_fifos();

#endif //FUNCIONES_SERVIDOR_H
