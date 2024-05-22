#ifndef MAIN_H_
#define MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/general.h>

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

extern t_log* logger;
extern t_config* config;
extern t_registros* registros;

t_registros* create_registros(void);
void log_registers();

void cpu(int);
t_list* fetch(int);
t_intruction_execute* decode(t_list*);
void exec(t_intruction_execute*);
void check_interrupt(void);

set_instruction mapInstruction(char*);
void* getRegister(char*);
void setTo(char*, char*);
void agregar_uint_a_paquete(t_paquete*, void*, int); // pasar a utils

#endif