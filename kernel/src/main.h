#ifndef MAIN_H_
#define MAIN_H_

#include <semaphore.h>
#include <readline/readline.h>
#include <commons/collections/queue.h>
#include <commons/temporal.h>
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
#define KEY_GRADO_MULTIPROGRAMACION "GRADO_MULTIPROGRAMACION"
#define KEY_RECURSOS "RECURSOS"
#define KEY_INSTANCIAS_RECURSOS "INSTANCIAS_RECURSOS"

typedef struct {
	uint32_t pid;
	uint32_t quantum;
} t_quantum; // Kenrel

typedef struct {
	uint32_t pid;
	op_code type_interrupt;
} t_interrupt;

typedef struct {
	int cant_instancias;
	t_queue* queue_waiting;
} t_recursos;

typedef struct {
	t_pcb* pcb;
	t_io* io_info;
} t_io_queue;
typedef struct {
	sem_t hay_io;
	sem_t mutex_io;
	t_queue* queue_io;
} t_io_client;

extern t_config* config;
extern t_log* logger;

extern int memoria_fd;

extern sem_t mutex_io_clients;
extern sem_t mutex_new;
extern sem_t mutex_ready;
extern sem_t mutex_interrupt;
extern sem_t mutex_recurso;

extern sem_t hay_ready;
extern sem_t hay_new;

extern sem_t hay_interrupt;
extern sem_t cont_multi;
extern sem_t plani_run;

extern uint32_t next_pid;
extern t_interrupt* interrupt_pid;

extern t_queue* queue_ready;
extern t_queue* queue_priority_ready;
extern t_queue* queue_new;
extern uint32_t running_pid;

extern t_dictionary* dict_recursos;
extern t_dictionary* dict_io_clients;

extern uint32_t quantum;
extern bool isStopped;
extern uint32_t multiprogramacion;

void* largo_main(void*);
void* corto_main(void*);
void* io_main(void*);
void* consola_main(void*);
void* interrupt_main(void*);
void* memoria_finalizar_proceso(void*);
void* quantum_thread(void*);
void* io_client(void*);

void iniciar_proceso(int, char*, char*);
void enviar_new(void);
t_pcb* crear_context(uint32_t);
void enviar_cpu(int, t_pcb*);
t_pcb* esperar_cpu(int, t_temporal*);
void finalizar_proceso(t_pcb*);
pthread_t create_quantum_thread(t_quantum*);
void finalizar_pid(uint32_t);
void ejecutar_script(int, char*, char*);
void cleanup_thread(void*);

void* multi_change_waits(void*);
void cambiar_multiprogramacion(uint32_t);
void check_plani(void);

#endif

/*
	Arreglar el RR y agregar el RR (dejar que mueran o matar hilos) usar de commons el times
	Aceptar varias I/O con su respectivo nombre (Manejar que no exista la interfaz pedida de CPU)
	Planificador largo plazo (maximo de proceso y eliminar un programa)
	Manejar recursos (menos prioridad)

	COnsultar como aumentar el tamanio maximo de un semafor ya inicializado, con destroy y eso se puede pero que pasa
	con los que estabann esperando ?

	REVISAR NOMBRE DE PCB STATUS, separar en:
		RUNNING_QUANTUM -> Si va a cola de prioridad o no
		RUNNING_SIGNAL -> Va a cola de prioridad en el primer lugar (tiene que volver a correr) y pasa a cola normal ready un recurso que haya pedido
		BLOCKED_IO -> Agregar a cola de blocked del nombre de entrada salida o si no existe EXIT
		BLOCKED_WAIT -> Agregar a la cola del waiting para el recurso pedido si existe, sino EXIT

*/