#ifndef UTILS_MEM_H_
#define UTILS_MEM_H_

#include "general_protocol.h"

typedef struct {
	uint32_t pid;
	char* path;
} t_init_pid;

t_init_pid* recibir_init_process(int, t_log*);
void agregar_init_process_paquete(t_paquete*, uint32_t, char*);
void* recibir_mem_write(int, uint32_t*, int*, t_log*);
void* recibir_mem_read(int, uint32_t*, int*, t_log*);
uint32_t recibir_pid_con_uint32(int, uint32_t*, t_log*);

#endif