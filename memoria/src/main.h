#ifndef MAIN_H_
#define MAIN_H_

#include <utils/mem_protocol.h>
#include <commons/collections/dictionary.h>
#include <commons/bitarray.h>
#include <commons/memory.h>
#include <semaphore.h>

// FILES
#define CONFIG_FILE "memoria.config"
#define LOGS_FILE "memoria.logs"

// KEYS_CONFIG_FILE
#define KEY_CLIENT_LOG "LOGGER_CLIENTE"
#define KEY_SERVER_LOG "LOGGER_SERVIDOR"
#define KEY_PUERTO_ESCUCHA "PUERTO_ESCUCHA"

#define KEY_RETARDO_RESPUESTA "RETARDO_RESPUESTA"
#define KEY_TAM_MEMORIA "TAM_MEMORIA"
#define KEY_TAM_PAGINA "TAM_PAGINA"
#define KEY_PATH_INSTRUCCIONES "PATH_INSTRUCCIONES"

typedef struct {
	uint32_t pid;
    char** file;
    t_list* pages; 
} t_memoria;


extern uint32_t page_size;
extern uint32_t max_pages;

extern void* memoria_usuario; 

extern t_bitarray* bit_map;
extern t_dictionary* memoria_procesos;

extern t_log* logger;
extern t_config* config;

extern char* path_scripts;

extern sem_t mutex_bit_map;
extern sem_t mutex_mem_usuario;
extern sem_t mutex_mem_procesos;

void* memoria(void*);
void retardo(void);
void liberar_memoria(int);
void init_proceso(int);
void init_script(int);

void resize_process(int);
op_code resize_up(int, t_list*);
void resize_down(int, t_list*);
void escribir_memoria(int);
void leer_memoria(int);

FILE* open_file(char*);
int get_file_size(FILE*);
op_code leer_script(char*, char**);
op_code leer_archivo(uint32_t, char*);
void get_instruction(int);
void get_pid_page(int);
t_memoria* get_pid_mem(uint32_t);
void enviar_instruccion(int, char**);
char** get_instruction_pid(uint32_t, uint32_t);

#endif
 /* 
    Revisar finalizacion de proceso, no tengo los bitmaps libes
    p.d Creo que lo arreglue poniendo pages >= 0 en vez de pages > 0 (no permite resize down a 0)
        probar que no rompa los resize de instruccion y que funcione para bitmap

*/