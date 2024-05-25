#ifndef UTILS_GENERAL_H_
#define UTILS_GENERAL_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/string.h>
#include <readline/readline.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

typedef enum {
	NEW,
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
	INIT_PID_SUCCESS,
	INIT_PID_ERROR,
	INTERRUPT
} op_code;

typedef enum {
	SET,
    SUM,
    SUB,
    JNZ,
    IO_GEN_SLEEP,
	EXIT,
	UNKNOWN,
} set_instruction;

typedef struct {
    uint32_t pc, si, di;
    uint8_t ax, bx, cx, dx;
    uint32_t eax, ebx, ecx, edx;
} t_registros;

typedef struct {
	uint32_t pid, pc, quantum;
	t_registros* registers;
	pid_status status;
} t_pcb;
typedef struct{
	int size;
	void* stream;
} t_buffer;

typedef struct {
	char* name_interface;
	set_instruction type_instruction;
	uint32_t sleep_time;
} t_io;

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
