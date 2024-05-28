#ifndef UTILS_MEM_H_
#define UTILS_MEM_H_

#include "general_protocol.h"

typedef struct {
	uint32_t pid;
	char* path;
} t_init_pid;

t_init_pid* recibir_init_process(int, t_log*);
void agregar_init_process_paquete(t_paquete*, uint32_t, char*);

#endif