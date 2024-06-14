#include "main.h"

void finalizar_proceso(t_pcb* pcb){ 
    pthread_t exit_thread;
    uint32_t pid = pcb->pid;
    
    if (pthread_create(&exit_thread, NULL, memoria_finalizar_proceso, &pid)) {
        log_error(logger, "No se pudo crear thread para finilzar proceso");
        exit(EXIT_FAILURE);
    } else {
        pthread_detach(exit_thread);
    }

    free(pcb->registers);
    free(pcb);

    sem_post(&cont_multi);
}

void* memoria_finalizar_proceso(void* pid){
    uint32_t* pid_end = (uint32_t*) pid;
    op_code code = END_PID;

    log_info(logger, "Enviar PID: %u a finalizar", *(pid_end));

    send(memoria_fd, &code, sizeof(uint32_t), 0);
    send(memoria_fd, pid_end, sizeof(uint32_t), 0);

    return EXIT_SUCCESS;
}