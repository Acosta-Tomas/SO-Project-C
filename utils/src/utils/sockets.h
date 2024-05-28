#ifndef UTILS_SOCKETS_H_
#define UTILS_SCOKETS_H_

#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

// Sockets Server
int iniciar_servidor(char*);
int esperar_cliente(int);

// Sockets Client
int crear_conexion(char*, char*);
void liberar_conexion(int);


#endif