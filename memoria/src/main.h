#ifndef MAIN_H_
#define MAIN_H_

#include <utils/mem_protocol.h>
#include <commons/collections/dictionary.h>
#include <commons/bitarray.h>

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

typedef struct {
	uint32_t pid;
    char** file;
    t_list* pages; 
} t_memoria;


extern uint32_t page_size;
extern uint32_t max_pages;
extern void* memoria_usuario; 
extern t_bitarray* bit_map; // Neceista sem?
extern t_dictionary* memoria_procesos; // necesita sem?
extern t_log* logger;
extern t_config* config;

op_code leer_archivo(uint32_t, const char*);
void* memoria(void*);
char** get_instruction_pid(uint32_t, uint32_t);
void enviar_instruccion(int, char**);
void resize_process(int);
op_code resize_up(int, t_list*);
void resize_down(int, t_list*);
void retardo(void);
void get_pid_page(int);
void escribir_memoria(int);
void leer_memoria(int);

#endif
 /* 
 
    To Do: 
        Arreglar obtencion de instrucciones del pseudocodigo
        Agregar retardo x archivo de config para responder a cada peticion de instruccion y/o escritura, lectura de memoria de usuario
        Agregar tablas de paginas en la creación de un proceso
        Manejar el espacio contiguo de memoria
        Finalizacion de proceso, marcar frames libres y eliminar las instrucciones 
        Finalizar por completo el módulo (no olvidarse los logs)
 */

/*
    Resize -> Check bitmap  -> Frame libre -> Create page para el PID -> Marcar frame opcupad
                            -> No hay frames libres -> Memory error -> Exit

    PID page -> hay pagina? -> Devolver frame
                            -> Error

    Read -> DF y Bytes  -> Devolver lo leiod

    Write -> DF, dato   -> Escribir 

    END -> PID -> Clean frames

*/