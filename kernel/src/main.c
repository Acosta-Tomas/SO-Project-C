#include "main.h"

t_config* config;
t_log* logger;

pthread_t consola_thread, largo_thread, corto_thread, io_thread;

sem_t hay_ready;
sem_t mutex_ready;
sem_t hay_io;
sem_t mutex_io;
sem_t hay_new;
sem_t mutex_new;

t_queue* queue_ready;
t_queue* queue_io;
t_queue* queue_new;

int main(int argc, char* argv[]) {
    config = config_create(CONFIG_FILE);

    if (config == NULL) exit(EXIT_FAILURE); 
    
    logger = log_create(LOGS_FILE, config_get_string_value(config, KEY_LOGGER), false, LOG_LEVEL_DEBUG);

    sem_init(&hay_ready, 0, 0);    
    sem_init(&mutex_ready, 0, 1);
    sem_init(&hay_ready, 0, 0);    
    sem_init(&mutex_ready, 0, 1); 
    sem_init(&hay_new, 0, 0);    
    sem_init(&mutex_new, 0, 1); 

    queue_ready = queue_create();
    queue_io = queue_create();
    queue_new = queue_create();


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

    // Esperar hilos a finalizar
    pthread_join(largo_thread, NULL);
    pthread_join(corto_thread, NULL);
    pthread_join(io_thread, NULL);
    pthread_join(consola_thread, NULL);

    sem_destroy(&hay_ready);
    sem_destroy(&mutex_ready);
    sem_destroy(&hay_io);
    sem_destroy(&mutex_io);

    log_destroy(logger);
    config_destroy(config);
    return EXIT_SUCCESS;
}

// // Solo para probar threads, se incia y termina al toque
// void cliente_main(){
//     t_log* logger = log_create(LOGS_FILE, config_get_string_value(config, KEY_CLIENT_LOG), true, LOG_LEVEL_INFO);
//     log_info(logger, "Thread - Client for testing");

//     char* ip_cpu = config_get_string_value(config, KEY_IP_CPU);
//     char* puerto_cpu_dispatch = config_get_string_value(config, KEY_PUERTO_CPU_DISPATCH);
//     char* puerto_cpu_interrupt = config_get_string_value(config, KEY_PUERTO_CPU_INTERRUPT);

//     log_destroy(logger);
// }