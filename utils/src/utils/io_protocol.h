#ifndef UTILS_IO_H_
#define UTILS_IO_H_

#include "instruction_protocol.h"

typedef struct {
	set_instruction type_instruction;
	uint32_t buffer_size;
	void* buffer;
} t_io; // IO - Kernel

typedef struct {
	t_list* frames;
	int ptr;
	uint32_t size;
} t_fs_rw;

t_io* recibir_io(int, char**, t_log*);
void agregar_io_paquete(t_paquete*, set_instruction, char* params[], int);
void agregar_io_serializado(t_paquete*, t_io*);
t_io* recibir_io_serializado(int, t_log*);
uint32_t get_io_frames(t_io*, t_list*);
char* get_io_file(t_io*, char*);
char* get_io_truncate(t_io*, char*, int*);
char* get_io_read_write(t_io*, char*, t_fs_rw*);

#endif