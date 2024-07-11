#include "main.h"

uint32_t running_pid;

void* corto_main(void *arg){
    log_info(logger, "Thread planificado de corto plazo creado");

    char* algoritmo = config_get_string_value(config, KEY_ALGORITMO_PLANIFICACION);
    int hay_quantum = strcmp(algoritmo, "FIFO");
    int hay_priority = !strcmp(algoritmo, "VRR");

    char* ip_cpu = config_get_string_value(config, KEY_IP_CPU);
    char* puerto_cpu_dispatch = config_get_string_value(config, KEY_PUERTO_CPU_DISPATCH);
    int conexion = crear_conexion(ip_cpu, puerto_cpu_dispatch);


    log_info(logger, "Connected to CPU Dispatch - SOCKET: %d", conexion);

    while (1) {
        sem_wait(&hay_ready);
        sem_wait(&mutex_ready);
        t_pcb* pcb = queue_is_empty(queue_priority_ready) ? queue_pop(queue_ready) : queue_pop(queue_priority_ready);
        pcb->status = RUNNING;
        sem_post(&mutex_ready);

        pthread_t q_thread;
        t_quantum* pid_quantum = malloc(sizeof(t_quantum));

        pid_quantum->quantum =  pcb->quantum;
        pid_quantum->pid = pcb->pid;
        running_pid = pcb->pid;

        enviar_cpu(conexion, pcb);

        if(hay_quantum) q_thread = create_quantum_thread(pid_quantum);
        t_temporal* pid_timestamp = temporal_create();

        pcb = esperar_cpu(conexion);

        temporal_stop(pid_timestamp);
        uint32_t used_timestamp = (uint32_t) temporal_gettime(pid_timestamp);
        if (hay_priority) pcb->quantum = (pcb->quantum != quantum || used_timestamp >= pcb->quantum) ? quantum : quantum - used_timestamp;

        log_info(logger, "PID: %u - Recursos: %s", pcb->pid, pcb->recursos);

        if (pcb->status != RUNNING_QUANTUM && hay_quantum) {
            if (pthread_cancel(q_thread) != 0) {
                log_info(logger, "Error al cancelar quantum thread");
            }
        }

        if (pcb->status == TERMINATED) {
            log_info(logger, "Proceso Finalizado - PID: %u", pcb->pid);
            log_registers(pcb, logger);
            finalizar_proceso(pcb);
        }

        if (pcb->status == TERMINATED_USER) {
            log_info(logger, "Proceso Finalizado por usuario - PID: %u", pcb->pid);
            log_registers(pcb, logger);
            finalizar_proceso(pcb);
        }

        if (pcb->status == ERROR) {
            log_error(logger, "Error en proceso - PID: %u", pcb->pid);
            finalizar_proceso(pcb);
        }

        if (pcb->status == RUNNING) {
            log_info(logger, "Proceso - PID: %u", pcb->pid);

            sem_wait(&mutex_ready);
            pcb->quantum < quantum ? queue_push(queue_priority_ready, pcb) : queue_push(queue_ready, pcb);
            sem_post(&mutex_ready);
            sem_post(&hay_ready);
        }

        if (pcb->status == RUNNING_QUANTUM) {
            log_info(logger, "Proceso fin de Quantum - PID: %u", pcb->pid);

            sem_wait(&mutex_ready);
            queue_push(queue_ready, pcb);
            sem_post(&mutex_ready);
            sem_post(&hay_ready);
        }

        if (pcb->status == RUNNING_SIGNAL){
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

    log_info(logger, "Proceso enviado a CPU - PID: %u", pcb->pid);
    free(pcb->registers);
    free(pcb->recursos);
    free(pcb);
}

t_pcb* esperar_cpu(int conexion){
    t_pcb* pcb_updated;
    t_io* io_request;
    op_code cod_op = recibir_operacion(conexion);

    if (cod_op == PCB) 
        return recibir_pcb(conexion, logger);
    
    if (cod_op == IO){
        char* name_interface = NULL;
        io_request = recibir_io(conexion, &name_interface, logger);

        recibir_operacion(conexion);
        pcb_updated = recibir_pcb(conexion, logger);

        if(dictionary_has_key(dict_io_clients, name_interface)){
            t_io_client* io_client = dictionary_get(dict_io_clients, name_interface);
            t_io_queue* io_queue = malloc(sizeof(t_io_queue));

            io_queue->pcb = pcb_updated;
            io_queue->io_info = io_request;


            sem_wait(&io_client->mutex_io);
            queue_push(io_client->queue_io, io_queue);  
            sem_post(&io_client->mutex_io);
            sem_post(&io_client->hay_io);

            log_info(logger, "Proceso bloqueado - PID: %u", pcb_updated->pid);

            return pcb_updated;
        }

        pcb_updated->status = ERROR;

        return pcb_updated;
    }

    if (cod_op == WAIT_RECURSO){
        uint32_t size;
        int buffer_size;
        void* buffer;

        buffer = recibir_buffer(&buffer_size, conexion);

        memcpy(&size, buffer, sizeof(uint32_t));
        char* nombre_recurso = malloc(size);
        memcpy(nombre_recurso, buffer + sizeof(uint32_t), size);

        free(buffer);

        recibir_operacion(conexion);
        pcb_updated = recibir_pcb(conexion, logger);

        if(!dictionary_has_key(dict_recursos, nombre_recurso)) {
            pcb_updated->status = TERMINATED;
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
        free(nombre_recurso);

        return pcb_updated;
    }

    if (cod_op == SIGNAL_RECURSO){
        uint32_t size;
        int buffer_size;
        void* buffer;

        buffer = recibir_buffer(&buffer_size, conexion);

        memcpy(&size, buffer, sizeof(uint32_t));
        char* nombre_recurso = malloc(size);
        memcpy(nombre_recurso, buffer + sizeof(uint32_t), size);

        free(buffer);

        recibir_operacion(conexion);
        pcb_updated = recibir_pcb(conexion, logger);

        if(!dictionary_has_key(dict_recursos, nombre_recurso)) {
            pcb_updated->status = TERMINATED;
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

            sem_wait(&mutex_ready);
            queue_push(queue_ready, pcb_to_ready);
            sem_post(&mutex_ready);
            sem_post(&hay_ready);

        } else sem_post(&mutex_recurso);

        return pcb_updated;
    }

    return pcb_updated;
}
