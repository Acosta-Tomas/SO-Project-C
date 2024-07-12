#include "main.h"

t_config* config;
t_log* logger;

pthread_t consola_thread, largo_thread, corto_thread, io_thread, interrup_thread;

sem_t mutex_io_clients;
sem_t mutex_ready;
sem_t mutex_new;
sem_t mutex_interrupt;
sem_t mutex_recurso;

sem_t hay_ready;
sem_t hay_new;

sem_t hay_interrupt;
sem_t cont_multi;
sem_t plani_run;

t_queue* queue_priority_ready;
t_queue* queue_ready;
t_queue* queue_new;

t_dictionary* dict_recursos;
t_dictionary* dict_io_clients;

t_interrupt* interrupt_pid;
uint32_t multiprogramacion;

uint32_t quantum;
bool isStopped = false;

int main(int argc, char* argv[]) {
    config = config_create(CONFIG_FILE);

    if (config == NULL) exit(EXIT_FAILURE); 
    
    logger = log_create(LOGS_FILE, config_get_string_value(config, KEY_LOGGER), false, LOG_LEVEL_DEBUG);
    multiprogramacion = (uint32_t) config_get_int_value(config, KEY_GRADO_MULTIPROGRAMACION);
    
    sem_init(&mutex_io_clients, 0, 1); 
    sem_init(&mutex_ready, 0, 1);
    sem_init(&mutex_new, 0, 1);   
    sem_init(&mutex_interrupt, 0, 1);   
    sem_init(&mutex_recurso, 0, 1);
    sem_init(&hay_ready, 0, 0);    
    sem_init(&hay_new, 0, 0);    
    sem_init(&hay_interrupt, 0, 0);    
    sem_init(&cont_multi, 0, multiprogramacion);
    sem_init(&plani_run, 0, 1);    

    queue_priority_ready = queue_create();
    queue_ready = queue_create();
    queue_new = queue_create();
    
    interrupt_pid = malloc(sizeof(t_interrupt));

    dict_io_clients = dictionary_create();
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

    quantum = (uint32_t) config_get_int_value(config, KEY_QUANTUM);

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

    if (pthread_create(&interrup_thread, NULL, interrupt_main, NULL)) {
        log_error(logger, "Problema al crear hilo para el manejo de Interrupt por Quantum");
        exit(EXIT_FAILURE);
    }

    
    // Esperar hilos a finalizar
    pthread_join(largo_thread, NULL);
    pthread_join(corto_thread, NULL);
    pthread_join(io_thread, NULL);
    pthread_join(consola_thread, NULL);
    pthread_join(interrup_thread, NULL);

    sem_destroy(&hay_ready);
    sem_destroy(&mutex_ready);
    sem_destroy(&mutex_interrupt);
    sem_destroy(&mutex_recurso);
    sem_destroy(&hay_new);
    sem_destroy(&mutex_ready);
    sem_destroy(&mutex_io_clients);
    sem_destroy(&hay_interrupt);
    sem_destroy(&cont_multi);
    sem_destroy(&plani_run);

    free(interrupt_pid);

    queue_destroy(queue_ready);
    queue_destroy(queue_new);
    queue_destroy(queue_priority_ready);

    // Eliminar queue y list de adentro de cada uno
    dictionary_destroy(dict_recursos);
    dictionary_destroy(dict_io_clients);
    log_destroy(logger);
    config_destroy(config);
    return EXIT_SUCCESS;
}


/*
    La idea es que se chequee en la entrada de cada etapa,
    Cuando vuelve de exec hay que chequear en las condiciones para pausar exactamente ahi
    Cuando vuelve de io lo mismo.
    En new, ready y end se chequea al entrar, si pasa continua.
*/
void check_plani(void) {
    sem_wait(&plani_run);
    sem_post(&plani_run);
}
