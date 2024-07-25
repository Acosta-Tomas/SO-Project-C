#include "main.h"

void* largo_main(void *arg){
    log_debug(logger, "Thread planificado de largo plazo creado");

    while (1){
        sem_wait(&cont_multi);
        sem_wait(&hay_new);

        check_plani();

        sem_wait(&mutex_new);
        t_pcb* pcb = queue_pop(queue_new);
        sem_post(&mutex_new);

        cambio_estado(pcb, READY);

        sem_wait(&mutex_ready);
        queue_push(queue_ready, pcb);
        sem_post(&mutex_ready);
        
        char* read_list = string_new();
        void log_pid(void* pcb) {
            t_pcb* pcb_print = (t_pcb*) pcb;

            string_append(&read_list, string_itoa((int) pcb_print->pid));
            string_append(&read_list, ", ");
        }

        list_iterate(queue_priority_ready->elements, &log_pid);
        list_iterate(queue_ready->elements, &log_pid);

        log_info(logger, "Cola Ready / Ready Prioridad: %s", read_list);
        free(read_list);
        sem_post(&hay_ready);
    }
    
    return EXIT_SUCCESS;
}