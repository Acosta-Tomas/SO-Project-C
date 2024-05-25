#include <utils/client.h>

void* serializar_paquete(t_paquete* paquete, int bytes){
	// bytes tiene la cantidad del sizde del stream + size(codigo op) +  size(buffer)
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	// se agrega codifo de op
	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	// Se agrega tamaño de lo que vamos a enviar
	desplazamiento+= sizeof(int);
	// Se agrega el stream de lo quevamos a enviar
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

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

void enviar_mensaje(char* mensaje, int socket_cliente){
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	// Todo paquete tiene el int del OP + el int del size 
	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}


void crear_buffer(t_paquete* paquete){
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(op_code op){
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = op;
	crear_buffer(paquete);
	
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio){
	// Se mueve con un tamaño nuevo de -> tamaño actual del stream (buffer_size) + tamaño de lo que viene (tamanio) + int (para agregar de cuanto es el tamaño nuevo de lo que viene)
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	// se copia al stream inmediatamente despues de lo ultimo el nuevo size de lo que se viene
	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	// Se copia despues de size el valor de lo que queremos mandar
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	// Se agrega al buffer el tamaño del size y el nuevo valor
	paquete->buffer->size += tamanio + sizeof(int); 
}

void agregar_uint_a_paquete(t_paquete* paquete, void* valor, int tamanio){

    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio);

    memcpy(paquete->buffer->stream + paquete->buffer->size, valor, tamanio);

    paquete->buffer->size += tamanio; 
}

void enviar_paquete(t_paquete* paquete, int socket_cliente){
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete){
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente){
	close(socket_cliente);
}

void terminar_programa(int conexion, t_log* logger, t_config* config){
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
}

void agregar_pcb_paquete(t_paquete* paquete, t_pcb* pcb){
    agregar_uint_a_paquete(paquete, &pcb->pid, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->pc, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->quantum, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->status, sizeof(pid_status));

    // registros
    agregar_uint_a_paquete(paquete, &pcb->registers->ax, sizeof(uint8_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->bx, sizeof(uint8_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->cx, sizeof(uint8_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->dx, sizeof(uint8_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->eax, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->ebx, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->ecx, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->edx, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->si, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->di, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->pc, sizeof(uint32_t));
}

void agregar_init_process_paquete(t_paquete* paquete, uint32_t pid, char* path){
	agregar_uint_a_paquete(paquete, &pid, sizeof(uint32_t));
	agregar_a_paquete(paquete, path, strlen(path) + 1);
}