//
// Created by erick on 3/28/26.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
//#include "headers/cliente.h"

#include <sys/stat.h>

#include "headers/funciones_servidor.h"

// ── Prototipos locales
int is_server_run();
int verificar_usuario_existe(const char *nombre_buscar);
void *leer_inbox(void *arg);

int corriendo = 1;

int main(void) {

    int estado_servidor = is_server_run();
    if (estado_servidor != 1) {
        printf("SERVIDOR NO INICIADO\n");
        return 1;
    }

    printf("--- BIENVENIDO AL CHAT ---\n");

    // 2. Pedir nombre
    datos_login yo;
    yo.pid = getpid();

    // 3. Enviar datos de login al servidor por el FIFO

    int fd_login = open("../data/login/login_requests", O_WRONLY);
    if (fd_login == -1) {
        perror("El servidor no acepta conexiones");
        return 1;
    }

    int nombre_valido = 0;
    while (!nombre_valido) {
        printf("Ingrese su nombre: ");
        if (scanf(" %19s", yo.nombre) != 1) { return 1; }

        write(fd_login, &yo, sizeof(datos_login));

        sleep(2);
        int confirmacion = open("../data/login/confirmacion", O_RDONLY);
        sleep(1);

        if (confirmacion == 0) {
            printf("Error: El nombre '%s' ya esta en uso. Intente otro.\n", yo.nombre);
        } else {
            nombre_valido = 1;
        }

        close(confirmacion);
    }
    close(fd_login);

    printf("Conectado como '%s' (PID: %d)\n", yo.nombre, yo.pid);
    printf("\nComandos:\n /usuarios\n  /privado\n  /global\n  /salir\n\n");



    // Rutas de este cliente
    char ruta_outbox[100], ruta_inbox[100];
    snprintf(ruta_outbox, sizeof(ruta_outbox), "../data/outbox/cliente%d", yo.pid);
    snprintf(ruta_inbox,  sizeof(ruta_inbox),  "../data/inbox/cliente%d",  yo.pid);

    // Esperar a que el servidor cree los archivos
    sleep(1);

    int outbox = open(ruta_outbox, O_WRONLY);

    // 4. Bucle principal
    char comando[20];

    pthread_t hilo;

    if (pthread_create(&hilo, NULL, leer_inbox, &yo.pid) != 0) {
        perror("Error al crear el hilo");
        return 1;
    }

    while (corriendo) {

        printf("\n¿Comando?: ");
        if (scanf(" %19s", comando) != 1) break;

        datos_operacion op;
        memset(&op, 0, sizeof(datos_operacion));
        strncpy(op.emisor, yo.nombre, sizeof(op.emisor) - 1);
        strncpy(op.comando, comando, sizeof(op.comando) - 1);

        // ── /usuarios ──────────────────────────────────────────
        if (strcmp(comando, "/usuarios") == 0) {
            // Solo escribir la operación al outbox; el servidor responde al inbox
            write(outbox, &op, sizeof(datos_operacion));

        // ── /privado ───────────────────────────────────────────
        } else if (strcmp(comando, "/privado") == 0) {
            printf("¿A quién?: ");
            scanf(" %19s", op.receptor);

            if (!verificar_usuario_existe(op.receptor)) {
                printf("Error: '%s' no esta conectado.\n", op.receptor);
                continue;
            }

            printf("Mensaje: ");
            scanf(" %49[^\n]", op.mensaje);

            write(outbox, &op, sizeof(datos_operacion));
            printf("Mensaje enviado.\n");

        // ── /global ────────────────────────────────────────────
        } else if (strcmp(comando, "/global") == 0) {
            printf("Mensaje para todos: ");
            scanf(" %49[^\n]", op.mensaje);

            write(outbox, &op, sizeof(datos_operacion));

        // ── /salir ─────────────────────────────────────────────
        } else if (strcmp(comando, "/salir") == 0) {
            write(outbox, &op, sizeof(datos_operacion));
            printf("Cerrando sesion...\n");
            sleep(1);
            corriendo = 0;

            if (pthread_join(hilo, NULL) != 0) {
                perror("Error al esperar el hilo");
                return 1;
            }

        } else {
            printf("Comando no reconocido. Usa:\n /usuarios\n /privado\n /global\n /salir\n");
        }
    }

    close(outbox);

    return 0;

}

int is_server_run() {

    //IMPORTANTE: prueba que el proceso servidor está en ejecución

    FILE *status = fopen("../data/server_status.log", "rb");
    if (status == NULL) {perror("error al abrir"); return 1;}
    int buffer;
    fread(&buffer, sizeof(int), 1, status);
    fclose(status);
    return buffer;

}

// ── Verificar si un usuario ya existe en usuarios.bin ────────────
int verificar_usuario_existe(const char *nombre_buscar) {
    FILE *archivo = fopen("../data/usuarios.bin", "rb");
    if (!archivo) return 0;

    datos_login registro;
    int encontrado = 0;
    while (fread(&registro, sizeof(datos_login), 1, archivo) == 1) {
        if (registro.pid != 0 && strcmp(registro.nombre, nombre_buscar) == 0) {
            encontrado = 1;
            break;
        }
    }
    fclose(archivo);
    return encontrado;
}

// ── Leer y mostrar mensajes del inbox ─────────────────────────────
void *leer_inbox(void *arg) {

    pid_t pid = *(pid_t*)arg;
    char ruta[100];
    snprintf(ruta, sizeof(ruta), "../data/inbox/cliente%d", pid);

    int f = open(ruta, O_RDONLY);
    if (!f) return;
    char mensaje[100];

    while (corriendo) {
        ssize_t n = read(f, &mensaje, sizeof(mensaje));
        if (n > 0) {
            if (strcmp(mensaje, "[Servidor] Apagando el sistema\n") == 0) {
                corriendo = 0;
            }
            printf("Nuevo Mensaje\n %s\n", mensaje);
        }
    }

    close(f);

}
