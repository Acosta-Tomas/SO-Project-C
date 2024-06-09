#ifndef UTILS_GENERAL_H_
#define UTILS_GENERAL_H_

#include <pthread.h>
#include <assert.h>

#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/string.h>

#include "sockets.h"

typedef enum{
	MENSAJE,
	PAQUETE,
	INSTRUCTION,
	PCB,
	IO,
	IO_SUCCESS,
	IO_ERROR,
	INIT_PID,
	INIT_PID_SUCCESS,
	INIT_PID_ERROR,
	INTERRUPT,
	MEM_RESIZE,
	MEM_READ,
	MEM_WRITE,
	MEM_PID_PAGE,
} op_code;

typedef struct{
	int size;
	void* stream;
} t_buffer;

typedef struct{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

// Protocolo Server
t_list* recibir_paquete(int);
void* recibir_buffer(int*, int);
void recibir_mensaje(int, t_log*);
int recibir_operacion(int);

// Protocolo Client
t_paquete* crear_paquete(op_code);
void enviar_paquete(t_paquete*, int);
void agregar_a_paquete(t_paquete*, void*, int);
void agregar_uint_a_paquete(t_paquete*, void*, int);
void eliminar_paquete(t_paquete*);

void enviar_mensaje(char*, int);


#endif
