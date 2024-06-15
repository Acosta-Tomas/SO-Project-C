#include "general_protocol.h"

// SERVER

int recibir_operacion(int socket_cliente){
	int cod_op;

	// Recibir codigo operacion
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0) return cod_op;
	else{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente){
	void * buffer;

	// Recibo el tamaño de lo que vamos a leer
	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	// Guardo espacio para ese tamaño y espero que lo manden
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

char* recibir_mensaje(int socket_cliente){
	int size;
	
	char* buffer = recibir_buffer(&size, socket_cliente); // Sabemos que es char pq es un msj string
	char* mensaje = string_duplicate(buffer);

	free(buffer);

	return mensaje;
}

t_list* recibir_paquete(int socket_cliente){
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	// Recibimos el bufffer entero serialiado
	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int)); // Copiamos el size del dato (es un int siempre)
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio); // reservamos memoria para ese dato que viene
		memcpy(valor, buffer+desplazamiento, tamanio); // guardamos el valor 
		desplazamiento+=tamanio;
		list_add(valores, valor);	// Agregamos a la lista
	}
	free(buffer);
	return valores;
}

// CLIENT

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