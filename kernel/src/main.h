#ifndef MAIN_H_
#define MAIN_H_

#include <semaphore.h>
#include <readline/readline.h>
#include <commons/collections/queue.h>
#include <utils/pcb_protocol.h>
#include <utils/io_protocol.h>
#include <utils/mem_protocol.h>

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
#define KEY_ALGORITMO_PLANIFICACION "ALGORITMO_PLANIFICACION"
#define KEY_QUANTUM "QUANTUM"

typedef struct {
	uint32_t pid;
	uint32_t quantum;
} t_quantum; // Kenrel

extern t_config* config;
extern t_log* logger;

extern sem_t hay_ready;
extern sem_t mutex_ready;
extern sem_t hay_io;
extern sem_t mutex_io;
extern sem_t hay_new;
extern sem_t mutex_new;
extern sem_t mutex_blocked;
extern sem_t start_quantum;

extern t_queue* queue_ready;
extern t_queue* queue_io;
extern t_queue* queue_new;
extern t_queue* queue_blocked;
extern uint32_t next_pid;
extern t_quantum* running_pid;


void* largo_main(void *arg);
void* corto_main(void *arg);
void* io_main(void *arg);
void* consola_main(void *arg);
void* quantum_main(void *arg);

op_code iniciar_proceso(int, char*, char*);
void enviar_new(void);
t_pcb* crear_context(uint32_t);
void enviar_cpu(int, t_pcb*);
t_pcb* esperar_cpu(int);

#endif

/*
	Arreglar el RR y agregar el RR (dejar que mueran o matar hilos) usar de commons el times
	Aceptar varias I/O con su respectivo nombre (Manejar que no exista la interfaz pedida de CPU)
	Planificador largo plazo (maximo de proceso y eliminar un programa)
	Manejar recursos (menos prioridad)
*/