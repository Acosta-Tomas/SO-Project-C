#include "sockets.h"

// Server

int iniciar_servidor(char* puerto){
	int socket_servidor;
    int err;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	err = getaddrinfo(NULL, puerto, &hints, &servinfo);

    if (err != 0) exit(EXIT_FAILURE);

	// Creamos el socket de escucha del servido
	socket_servidor = socket(servinfo->ai_family, 
								servinfo->ai_socktype, 
								servinfo->ai_protocol);

	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);
	// Escuchamos las conexiones entrantes
	listen(socket_servidor, 2);

	freeaddrinfo(servinfo);

	return socket_servidor;
}

int esperar_cliente(int socket_servidor){
	int socket_cliente = accept(socket_servidor, NULL, NULL);
	return socket_cliente;
}

// Client

int crear_conexion(char *ip, char* puerto){
	struct addrinfo hints, *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family, 
								server_info->ai_socktype, 
								server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);


	freeaddrinfo(server_info);

	return socket_cliente;
}

void liberar_conexion(int socket_cliente){
	close(socket_cliente);
}
