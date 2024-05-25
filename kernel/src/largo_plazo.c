#include "main.h"

void* largo_main(void *arg){
    log_info(logger, "Thread planificado de largo plazo creado");

    while (1){
        sem_wait(&hay_new);
        sem_wait(&mutex_new);
        t_pcb* pcb = queue_pop(queue_new);
        sem_post(&mutex_new);
        
        sem_wait(&mutex_ready);
        queue_push(queue_ready, pcb);
        sem_post(&mutex_ready);
        sem_post(&hay_ready);
    }
    
    return EXIT_SUCCESS;
}