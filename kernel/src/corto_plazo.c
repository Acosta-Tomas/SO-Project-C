#include "main.h"

int running_pid = -1;

void* corto_main(void *arg){
    log_debug(logger, "Thread planificado de corto plazo creado");

    char* algoritmo = config_get_string_value(config, KEY_ALGORITMO_PLANIFICACION);
    int hay_quantum = strcmp(algoritmo, "FIFO");
    int hay_priority = !strcmp(algoritmo, "VRR");

    char* ip_cpu = config_get_string_value(config, KEY_IP_CPU);
    char* puerto_cpu_dispatch = config_get_string_value(config, KEY_PUERTO_CPU_DISPATCH);
    int conexion = crear_conexion(ip_cpu, puerto_cpu_dispatch);


    log_info(logger, "SOCKET: %d - CPU Dispatch", conexion);

    while (1) {
        sem_wait(&hay_ready);

        check_plani();

        sem_wait(&mutex_ready);
        t_pcb* pcb = queue_is_empty(queue_priority_ready) ? queue_pop(queue_ready) : queue_pop(queue_priority_ready);
        sem_post(&mutex_ready);

        cambio_estado(pcb, RUNNING);

        pthread_t q_thread;
        t_quantum* pid_quantum = malloc(sizeof(t_quantum));

        pid_quantum->quantum =  pcb->quantum;
        pid_quantum->pid = pcb->pid;
        running_pid = (int) pcb->pid;

        enviar_cpu(conexion, pcb);

        if(hay_quantum) q_thread = create_quantum_thread(pid_quantum);
        t_temporal* pid_timestamp = temporal_create();

        pcb = esperar_cpu(conexion, pid_timestamp);

        uint32_t used_timestamp = (uint32_t) temporal_gettime(pid_timestamp);
        if (hay_priority) pcb->quantum = (pcb->quantum != quantum || used_timestamp >= pcb->quantum) ? quantum : quantum - used_timestamp;

        if (pcb->status != RUNNING_QUANTUM && hay_quantum) {
            if (pthread_cancel(q_thread) != 0) {
                log_debug(logger, "Error al cancelar quantum thread");
            }
        }

        if (pcb->status == TERMINATED) {
            pcb->status = RUNNING;
            finalizar_proceso(pcb, "SUCCESS");
        }

        if (pcb->status == TERMINATED_USER) finalizar_proceso(pcb, "INTERRUPTED_BY_USER");

        if (pcb->status == ERROR) finalizar_proceso(pcb, "ERROR");

        if (pcb->status == ERROR_RESOURCE) finalizar_proceso(pcb, "INVALID_RESOURCE");

        if (pcb->status == ERROR_INTERFACE) finalizar_proceso(pcb, "INVALID_INTERFACE");

        if (pcb->status == ERROR_MEMORY) {
            pcb->status = RUNNING;
            finalizar_proceso(pcb, "OUT_OF_MEMORY");
        }

        if (pcb->status == RUNNING) {
            cambio_estado(pcb, READY);

            sem_wait(&mutex_ready);
            pcb->quantum < quantum ? queue_push(queue_priority_ready, pcb) : queue_push(queue_ready, pcb);
            sem_post(&mutex_ready);
            sem_post(&hay_ready);
        }

        if (pcb->status == RUNNING_QUANTUM) {
            log_info(logger, "PID: %u - Desalojado por fin de Quantum", pcb->pid);
            cambio_estado(pcb, READY);

            sem_wait(&mutex_ready);
            queue_push(queue_ready, pcb);
            sem_post(&mutex_ready);
            sem_post(&hay_ready);
        }

        if (pcb->status == RUNNING_SIGNAL){
            cambio_estado(pcb, READY);

            sem_wait(&mutex_ready);
            queue_push(queue_priority_ready, pcb);
            sem_post(&mutex_ready);
            sem_post(&hay_ready);
        }

        temporal_destroy(pid_timestamp);
        running_pid = -1;
    }

    liberar_conexion(conexion);
    
    return EXIT_SUCCESS;
}

void enviar_cpu(int conexion, t_pcb* pcb){
    t_paquete* paquete = crear_paquete(PCB);

    agregar_pcb_paquete(paquete, pcb);
    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);

    log_debug(logger, "Proceso enviado a CPU - PID: %u", pcb->pid);
    free(pcb->registers);
    free(pcb->recursos);
    free(pcb);
}

t_pcb* esperar_cpu(int conexion, t_temporal* pid_timestamp){
    t_pcb* pcb_updated;
    t_io* io_request;
    op_code cod_op = recibir_operacion(conexion);

    if (cod_op == PCB) {
        pcb_updated = recibir_pcb(conexion, logger);
        temporal_stop(pid_timestamp);
        check_plani();

        return pcb_updated;
    }
    
    if (cod_op == IO){
        char* name_interface = NULL;
        io_request = recibir_io(conexion, &name_interface, logger);

        recibir_operacion(conexion);
        pcb_updated = recibir_pcb(conexion, logger);
        temporal_stop(pid_timestamp);
        check_plani();

        log_info(logger, "PID: %u - Bloqueado por: %s", pcb_updated->pid, name_interface);

        if(dictionary_has_key(dict_io_clients, name_interface)){
            t_io_client* io_client = dictionary_get(dict_io_clients, name_interface);
            t_io_queue* io_queue = malloc(sizeof(t_io_queue));

            io_queue->pcb = pcb_updated;
            io_queue->io_info = io_request;

            pcb_updated->status = RUNNING;
            cambio_estado(pcb_updated, BLOCKED_IO);

            sem_wait(&io_client->mutex_io);
            queue_push(io_client->queue_io, io_queue);  
            sem_post(&io_client->mutex_io);
            sem_post(&io_client->hay_io);

            free(name_interface);

            return pcb_updated;
        }

        pcb_updated->status = ERROR_INTERFACE;

        free(name_interface);
        return pcb_updated;
    }

    if (cod_op == WAIT_RECURSO){
        char* nombre_recurso = recibir_mensaje(conexion);

        recibir_operacion(conexion);
        pcb_updated = recibir_pcb(conexion, logger);
        temporal_stop(pid_timestamp);
        check_plani();

        if(!dictionary_has_key(dict_recursos, nombre_recurso)) {
            pcb_updated->status = ERROR_RESOURCE;
            free(nombre_recurso);
            return pcb_updated;
        } 

        t_recursos* recurso = dictionary_get(dict_recursos, nombre_recurso);
        
        sem_wait(&mutex_recurso);
        recurso->cant_instancias -= 1;
        
        if(recurso->cant_instancias >= 0) {
            sem_post(&mutex_recurso);
            pcb_updated->status = RUNNING;
            string_append(&pcb_updated->recursos, nombre_recurso);
            string_append(&pcb_updated->recursos, ",");

            free(nombre_recurso);
            return pcb_updated;
        }
        
        queue_push(recurso->queue_waiting, pcb_updated);
        sem_post(&mutex_recurso);

        log_info(logger, "PID: %u - Bloqueado por: %s", pcb_updated->pid, nombre_recurso);
        pcb_updated->status = RUNNING;
        cambio_estado(pcb_updated, BLOCKED_WAIT);
        free(nombre_recurso);

        return pcb_updated;
    }

    if (cod_op == SIGNAL_RECURSO){
        char* nombre_recurso = recibir_mensaje(conexion);

        recibir_operacion(conexion);
        pcb_updated = recibir_pcb(conexion, logger);
        temporal_stop(pid_timestamp);
        check_plani();

        if(!dictionary_has_key(dict_recursos, nombre_recurso)) {
            pcb_updated->status = ERROR_RESOURCE;
            free(nombre_recurso);
            return pcb_updated;
        } 

        t_recursos* recurso = dictionary_get(dict_recursos, nombre_recurso);

        if (string_contains(pcb_updated->recursos, nombre_recurso)) {
            string_append(&nombre_recurso, ",");
            char* new_recursos = string_replace(pcb_updated->recursos, nombre_recurso, "");
            pcb_updated->recursos = new_recursos;
        }

        sem_wait(&mutex_recurso);
        recurso->cant_instancias += 1;

        if(!queue_is_empty(recurso->queue_waiting)) {
            t_pcb* pcb_to_ready = queue_pop(recurso->queue_waiting);
            sem_post(&mutex_recurso);

            cambio_estado(pcb_to_ready, READY);

            sem_wait(&mutex_ready);
            pcb_to_ready->quantum < quantum ? queue_push(queue_priority_ready, pcb_to_ready) : queue_push(queue_ready, pcb_to_ready);
            sem_post(&mutex_ready);
            sem_post(&hay_ready);

        } else sem_post(&mutex_recurso);

        free(nombre_recurso);
        return pcb_updated;
    }

    return pcb_updated;
}
