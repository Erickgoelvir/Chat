cmake_minimum_required(VERSION 3.31)
project(chat C)

set(CMAKE_C_STANDARD 11)

add_executable(servidor src/servidor.c
                src/funciones_servidor.c
                src/headers/funciones_servidor.h
)

add_executable(cliente src/cliente.c
                src/funciones_servidor.c
                src/headers/funciones_servidor.h
)