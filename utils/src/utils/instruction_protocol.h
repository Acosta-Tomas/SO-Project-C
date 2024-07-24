#ifndef UTILS_INSTRUCTION_H_
#define UTILS_IINSTRUCTION_H_

#include "general_protocol.h"
typedef enum {
	SET,
    SUM,
    SUB,
    JNZ,
    MOV_IN, 
    MOV_OUT, 
    RESIZE, 
    COPY_STRING, 
    WAIT,
    SIGNAL,
    IO_STDIN_READ, 
    IO_STDOUT_WRITE,
    IO_GEN_SLEEP,
    IO_FS_CREATE,
    IO_FS_DELETE,
    IO_FS_TRUNCATE,
    IO_FS_WRITE,
    IO_FS_READ,
	EXIT,
	UNKNOWN,
} set_instruction; // cpu - IO

typedef struct {
    uint32_t direccion_fisica;
    uint32_t bytes;
} t_memoria_fisica;



int escribir_memoria(int, void*, t_list*, t_log*, uint32_t);
int leer_memoria(int, void*, t_list*, t_log*, uint32_t);

#endif