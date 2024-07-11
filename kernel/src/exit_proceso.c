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

    log_info(logger, "Enviar PID: %u a finalizar", pcb_end->pid);

    send(memoria_fd, &code, sizeof(uint32_t), 0);
    send(memoria_fd, &pcb_end->pid, sizeof(uint32_t), 0);

    free(pcb_end->registers);
    free(pcb_end);

    return EXIT_SUCCESS;
}