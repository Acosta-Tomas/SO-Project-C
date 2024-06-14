#include "main.h"

t_config* config;
t_log* logger;

pthread_t consola_thread, largo_thread, corto_thread, io_thread, quatum_thread;

sem_t hay_ready;
sem_t mutex_ready;
sem_t hay_io;
sem_t mutex_io;
sem_t hay_new;
sem_t mutex_new;
sem_t mutex_blocked;
sem_t start_quantum;
sem_t cont_multi;

t_queue* queue_priority_ready;
t_queue* queue_ready;
t_queue* queue_io;
t_queue* queue_new;
t_queue* queue_blocked;

t_dictionary* dict_recursos;
t_quantum* running_pid;

int main(int argc, char* argv[]) {
    config = config_create(CONFIG_FILE);

    if (config == NULL) exit(EXIT_FAILURE); 
    
    logger = log_create(LOGS_FILE, config_get_string_value(config, KEY_LOGGER), false, LOG_LEVEL_DEBUG);

    char* algoritmo = config_get_string_value(config, KEY_ALGORITMO_PLANIFICACION);

    sem_init(&hay_ready, 0, 0);    
    sem_init(&mutex_ready, 0, 1);
    sem_init(&hay_ready, 0, 0);    
    sem_init(&mutex_ready, 0, 1); 
    sem_init(&hay_new, 0, 0);    
    sem_init(&mutex_new, 0, 1); 
    sem_init(&hay_io, 0, 0);    
    sem_init(&mutex_io, 0, 1); 
    sem_init(&mutex_blocked, 0, 1);
    sem_init(&start_quantum, 0, 0);    
    sem_init(&cont_multi, 0, config_get_int_value(config, KEY_ALGORITMO_PLANIFICACION)); 

    queue_priority_ready = queue_create();
    queue_ready = queue_create();
    queue_io = queue_create();
    queue_new = queue_create();
    queue_blocked = queue_create();
    running_pid = malloc(sizeof(t_quantum));
    dict_recursos = dictionary_create();

    char** recursos = config_get_array_value(config, KEY_RECURSOS);
    char** instancias = config_get_array_value(config, KEY_INSTANCIAS_RECURSOS);

    while(!string_array_is_empty(recursos)){
        t_recursos* recurso = malloc(sizeof(t_recursos));
        char* nombre_recurso = string_array_pop(recursos);
        char* cantida_recurso = string_array_pop(instancias);

        recurso->cant_instancias = atoi(cantida_recurso);
        recurso->queue_waiting = queue_create();

        log_info(logger, "Recurso: %s - instancias %s", nombre_recurso, cantida_recurso);

        dictionary_put(dict_recursos, nombre_recurso, recurso);
    }

    if (pthread_create(&largo_thread, NULL, largo_main, NULL)) {
        log_error(logger, "Problema al crear hilo para el planificador de largo plazo");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&corto_thread, NULL, corto_main, NULL)) {
        log_error(logger, "Problema al crear hilo para el planificador de largo plazo");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&io_thread, NULL, io_main, NULL)) {
        log_error(logger, "Problema al crear hilo para el planificador de largo plazo");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&consola_thread, NULL, consola_main, NULL)) {
        log_error(logger, "Problema al crear hilo para el planificador de largo plazo");
        exit(EXIT_FAILURE);
    }

    if (strcmp(algoritmo, "FIFO") != 0) {
        if (pthread_create(&quatum_thread, NULL, quantum_main, NULL)) {
            log_error(logger, "Problema al crear hilo para el manejo de Interrupt por Quantum");
            exit(EXIT_FAILURE);
        } else {
            pthread_detach(quatum_thread);
        }
    }

    free(running_pid);
    // Esperar hilos a finalizar
    pthread_join(largo_thread, NULL);
    pthread_join(corto_thread, NULL);
    pthread_join(io_thread, NULL);
    pthread_join(consola_thread, NULL);

    sem_destroy(&hay_ready);
    sem_destroy(&mutex_ready);
    sem_destroy(&hay_new);
    sem_destroy(&mutex_ready);
    sem_destroy(&hay_io);
    sem_destroy(&mutex_io);
    sem_destroy(&mutex_blocked);
    sem_destroy(&start_quantum);
    sem_destroy(&cont_multi);

    log_destroy(logger);
    config_destroy(config);
    return EXIT_SUCCESS;
}