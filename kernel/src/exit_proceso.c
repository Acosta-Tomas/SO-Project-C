#include "main.h"

void finalizar_proceso(t_pcb* pcb){ 
    pthread_t exit_thread;
    
    if (pthread_create(&exit_thread, NULL, memoria_finalizar_proceso, pcb)) {
        log_error(logger, "No se pudo crear thread para finilzar proceso");
        exit(EXIT_FAILURE);
    } else pthread_detach(exit_thread);

    sem_post(&cont_multi);
}

void* memoria_finalizar_proceso(void* pcb){
    t_pcb* pcb_end = (t_pcb*) pcb;
    op_code code = END_PID;

   if (strlen(pcb_end->recursos)) {
        char** recursos = string_split(pcb_end->recursos, ",");

        for (int i = 0; i < (string_array_size(recursos) - 1); i += 1) {
            t_recursos* recurso = dictionary_get(dict_recursos, recursos[i]);

            sem_wait(&mutex_recurso);
            recurso->cant_instancias += 1;

            if(!queue_is_empty(recurso->queue_waiting)) {
                t_pcb* pcb_to_ready = queue_pop(recurso->queue_waiting);
                sem_post(&mutex_recurso);
                sem_wait(&mutex_ready);
                queue_push(queue_ready, pcb_to_ready);
                sem_post(&mutex_ready);
                sem_post(&hay_ready);
    
            } else sem_post(&mutex_recurso);
        }
    }

    send(memoria_fd, &code, sizeof(uint32_t), 0);
    send(memoria_fd, &pcb_end->pid, sizeof(uint32_t), 0);

    free(pcb_end->registers);
    free(pcb_end->recursos);
    free(pcb_end);

    return EXIT_SUCCESS;
}