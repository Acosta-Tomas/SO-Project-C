#ifndef MAIN_H_
#define MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/general.h>
#include <commons/collections/queue.h>

// FILES
#define CONFIG_FILE "kernel.config"
#define LOGS_FILE "kernel.logs"

// KEYS_CONFIG_FILE
#define KEY_CLIENT_LOG "LOGGER_CLIENTE"
#define KEY_LOGGER "LOGGER"
#define KEY_PUERTO_ESCUCHA "PUERTO_ESCUCHA"
#define KEY_IP_CPU "IP_CPU"
#define KEY_PUERTO_CPU_DISPATCH "PUERTO_CPU_DISPATCH"
#define KEY_PUERTO_CPU_INTERRUPT "PUERTO_CPU_INTERRUPT"
#define KEY_IP_MEMORIA "IP_MEMORIA"
#define KEY_PUERTO_MEMORIA "PUERTO_MEMORIA"

extern t_config* config;
extern t_log* logger;
extern sem_t hay_ready;
extern sem_t mutex_ready;
extern sem_t hay_io;
extern sem_t mutex_io;
extern sem_t hay_new;
extern sem_t mutex_new;
extern t_queue* queue_ready;
extern t_queue* queue_io;
extern t_queue* queue_new;
extern uint32_t next_pid;

void* largo_main(void *arg);
void* corto_main(void *arg);
void* io_main(void *arg);
void* consola_main(void *arg);

op_code iniciar_proceso(int, char*, char*);
void enviar_new(void);
t_pcb* crear_context(uint32_t);
void enviar_cpu(int, t_pcb*);
t_pcb* esperar_cpu(int);

#endif