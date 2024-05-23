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

typedef enum {
	RUNNING,
    TERMINATED,
    BLOCKED,
    ERROR
} pid_status;
typedef enum{
	MENSAJE,
	PAQUETE,
	INSTRUCTION,
	PCB,
	IO,
	INIT_PID,
	INTERRUPT
} op_code;

typedef struct {
    uint32_t pc, si, di;
    uint8_t ax, bx, cx, dx;
    uint32_t eax, ebx, ecx, edx;
} t_registros;

typedef struct {
	uint32_t pid, pc, quantum;
	t_registros* registers;
	pid_status status;
} t_context;
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
