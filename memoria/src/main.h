#ifndef MAIN_H_
#define MAIN_H_

#include <utils/mem_protocol.h>

// FILES
#define CONFIG_FILE "memoria.config"
#define LOGS_FILE "memoria.logs"

// KEYS_CONFIG_FILE
#define KEY_CLIENT_LOG "LOGGER_CLIENTE"
#define KEY_SERVER_LOG "LOGGER_SERVIDOR"
#define KEY_PUERTO_ESCUCHA "PUERTO_ESCUCHA"

typedef struct {
	uint32_t pid;
    t_list* file; 
} t_memoria;

extern t_list* lista_memoria;
extern t_log* logger;
extern t_config* config;

op_code leer_archivo(uint32_t, const char*);
void* memoria(void*);
char** get_instruction_pid(uint32_t, uint32_t);
void enviar_instruccion(int, char**);

#endif