#ifndef MAIN_H_
#define MAIN_H_

#include <semaphore.h>
#include <utils/pcb_protocol.h>
#include <utils/io_protocol.h>

// FILES
#define CONFIG_FILE "cpu.config"
#define LOGS_FILE "cpu.logs"

// KEYS_CONFIG_FILE
#define KEY_CLIENT_LOG "LOGGER_CLIENTE"
#define KEY_SERVER_LOG "LOGGER_SERVIDOR"
#define KEY_PUERTO_ESCUCHA_DISPATCH "PUERTO_ESCUCHA_DISPATCH"
#define KEY_PUERTO_ESCUCHA_INTERRUPT "PUERTO_ESCUCHA_INTERRUPT"
#define KEY_IP_MEMORIA "IP_MEMORIA"
#define KEY_PUERTO_MEMORIA "PUERTO_MEMORIA"


#define PC "PC"
#define AX "AX"
#define BX "BX"
#define CX "CX"
#define DX "DX"
#define EAX "EAX"
#define EBX "EBX"
#define ECX "ECX"
#define EDX "EDX"
#define SI "SI"
#define DI "DI"

typedef enum {
    SUMA,
    RESTA
} cpu_operation;

typedef struct {
    set_instruction operation;
    int total_params;
    char* params[5];
} t_intruction_execute;

typedef struct {
    uint32_t direccion_fisica;
    uint32_t bytes;
} t_memoria_fisica;


extern t_log* logger;
extern t_config* config;
extern t_pcb* pcb;

extern bool has_interrupt;
extern uint32_t page_size;

extern sem_t mutex_interrupt;
// extern sem_t mutex_pcb;

void* dispatch(void*);
void* interrupt(void*);

void cpu(int, int);
t_list* fetch(int);
t_intruction_execute* decode(t_list*);
pid_status exec(t_intruction_execute*, int, int);
bool check_interrupt();

void* get_register(char*);

void update_register_uint32(char*, void (*update_function)(uint32_t*, uint32_t), uint32_t);
void update_register_uint8(char*, void (*update_function)(uint8_t*, uint8_t), uint8_t);
void operation_register_uint8(char*, char*, cpu_operation);
void operation_register_uint32(char*, char*, cpu_operation);
void set_registro_uint8(uint8_t*, uint8_t);
void set_registro_uint32(uint32_t*, uint32_t);
bool jnz_register(char*, char*);
pid_status enviar_io(int, t_intruction_execute*);
pid_status resize_process(int, char*);
pid_status mov_out(int, char*, char*);
pid_status mov_in(int, char*, char*);
pid_status copy_string(int, char*);

pid_status mmu(int, uint32_t, uint32_t, t_list*);
pid_status escribir_memoria(int, void*, t_list*);
pid_status leer_memoria(int, void*, t_list*);

void pc_plus_plus(uint32_t*, uint32_t);
uint8_t atouint8(char*);
uint32_t atouint32(char*);
unsigned long sizeof_register(char*);
set_instruction mapInstruction(char*);

#endif

/*
    Agregar instrucciones de:
        MOV_IN, 
        MOV_OUT, 
        RESIZE, OK
        COPY_STRING, 
        IO_STDIN_READ, 
        IO_STDOUT_WRITE.
        SIGNA, -> Si llego con manejador de recursos
        WAIT -> Si llego con manejador de recursos
*/