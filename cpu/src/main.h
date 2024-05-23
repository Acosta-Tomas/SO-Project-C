#ifndef MAIN_H_
#define MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/general.h>
#include <pthread.h>
#include <semaphore.h>

// FILES
#define CONFIG_FILE "cpu.config"
#define LOGS_FILE "cpu.logs"

// KEYS_CONFIG_FILE
#define KEY_CLIENT_LOG "LOGGER_CLIENTE"
#define KEY_SERVER_LOG "LOGGER_SERVIDOR"
#define KEY_PUERTO_ESCUCHA_DISPATCH "PUERTO_ESCUCHA_DISPATCH"
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

typedef struct {
    uint32_t pc, si, di;
    uint8_t ax, bx, cx, dx;
    uint32_t eax, ebx, ecx, edx;
} t_registros;

typedef struct {
    set_instruction operation;
    int total_params;
    char* params[5];
} t_intruction_execute;

typedef enum {
	RUNNING,
    TERMINATED,
    BLOCKED,
    ERROR
} cpu_status;

typedef enum {
    SUMA,
    RESTA
} cpu_operation;

extern t_log* logger;
extern t_config* config;
extern t_registros* registros;
extern sem_t run;
extern sem_t wait;
extern sem_t mutex_registros;

t_registros* create_registros(void);
void log_registers();

void cpu(int);
t_list* fetch(int);
t_intruction_execute* decode(t_list*);
cpu_status exec(t_intruction_execute*);
void check_interrupt(void);


void* get_register(char*);

void update_register_uint32(char*, void (*update_function)(uint32_t*, uint32_t), uint32_t);
void update_register_uint8(char*, void (*update_function)(uint8_t*, uint8_t), uint8_t);
void operation_register_uint8(char*, char*, cpu_operation);
void operation_register_uint32(char*, char*, cpu_operation);
void set_registro_uint8(uint8_t*, uint8_t);
void set_registro_uint32(uint32_t*, uint32_t);
bool jnz_register(char*, char*);

void pc_plus_plus(uint32_t*, uint32_t);
uint8_t atouint8(char*);
uint32_t atouint32(char*);
unsigned long sizeof_register(char*);
set_instruction mapInstruction(char*);

#endif