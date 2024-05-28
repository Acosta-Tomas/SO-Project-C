#ifndef UTILS_IO_H_
#define UTILS_IO_H_

#include "general_protocol.h"
#include "instruction_protocol.h"

typedef struct {
	char* name_interface;
	set_instruction type_instruction;
	char* sleep_time;
} t_io; // IO - Kernel


t_io* recibir_io(int, t_log*);
void agregar_io_paquete(t_paquete*, set_instruction, char*, char*);

#endif