#ifndef UTILS_GENERAL_H_
#define UTILS_GENERAL_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

typedef enum{
	MENSAJE,
	PAQUETE
} op_code;

typedef struct{
	int size;
	void* stream;
} t_buffer;

typedef struct{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

/**
* @fn    decir_hola
* @brief Imprime un saludo al nombre que se pase por parámetro por consola.
*/
void decir_hola(char*);
void leer_consola(t_log*);

#endif
