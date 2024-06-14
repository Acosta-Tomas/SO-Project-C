#include "main.h"

void* corto_main(void *arg){
    log_info(logger, "Thread planificado de corto plazo creado");

    char* algoritmo = config_get_string_value(config, KEY_ALGORITMO_PLANIFICACION);
    uint32_t quantum = (uint32_t) config_get_int_value(config, KEY_QUANTUM);
    int hay_interrupt = strcmp(algoritmo, "FIFO");

    char* ip_cpu = config_get_string_value(config, KEY_IP_CPU);
    char* puerto_cpu_dispatch = config_get_string_value(config, KEY_PUERTO_CPU_DISPATCH);
    int conexion = crear_conexion(ip_cpu, puerto_cpu_dispatch);


    log_info(logger, "Connected to CPU Dispatch - SOCKET: %d", conexion);

    while (1) {
        sem_wait(&hay_ready);
        sem_wait(&mutex_ready);
        t_pcb* pcb = queue_pop(queue_ready);
        pcb->status = RUNNING;
        sem_post(&mutex_ready);

        if(hay_interrupt != 0) {
            running_pid->pid = pcb->pid;
            running_pid->quantum = quantum;
            sem_post(&start_quantum);
        }

        enviar_cpu(conexion, pcb);

        pcb = esperar_cpu(conexion);

        if (pcb->status == TERMINATED) {
            log_info(logger, "Proceso Finalizado - PID: %u", pcb->pid);
            log_registers(pcb, logger);
            finalizar_proceso(pcb);
        }

        if (pcb->status == ERROR) {
            log_error(logger, "Error en proceso - PID: %u", pcb->pid);
            finalizar_proceso(pcb);
        }

        if (pcb->status == RUNNING) {
            log_info(logger, "Proceso fin de Quantum - PID: %u", pcb->pid);

            sem_wait(&mutex_ready);
            queue_push(queue_ready, pcb);
            sem_post(&mutex_ready);
            sem_post(&hay_ready);
        }

        if (pcb->status == BLOCKED) {
            log_info(logger, "Proceso bloqueado - PID: %u", pcb->pid);

            sem_wait(&mutex_blocked);
            queue_push(queue_blocked, pcb);
            sem_post(&mutex_blocked);
        }
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
    free(pcb);
}

t_pcb* esperar_cpu(int conexion){
    t_pcb* pcb_updated;
    t_io* io_request;
    op_code cod_op = recibir_operacion(conexion);

    if (cod_op == PCB) 
        pcb_updated = recibir_pcb(conexion, logger);
    
    if (cod_op == IO){
        io_request = recibir_io(conexion, logger);

        sem_wait(&mutex_io);
        queue_push(queue_io, io_request);
        sem_post(&mutex_io);
        sem_post(&hay_io);

        cod_op = recibir_operacion(conexion);
        pcb_updated = recibir_pcb(conexion, logger);
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

        cod_op = recibir_operacion(conexion);
        pcb_updated = recibir_pcb(conexion, logger);

        if(!dictionary_has_key(dict_recursos, nombre_recurso)) {
            pcb_updated->status = TERMINATED;
            return pcb_updated;
        } 

        t_recursos* recurso = dictionary_get(dict_recursos, nombre_recurso);

        if(recurso->cant_instancias >= 0) {
            recurso->cant_instancias -= 1;
            pcb_updated->status = RUNNING;

            return pcb_updated;
        }
        
        queue_push(recurso->queue_waiting, pcb_updated);
        pcb_updated->status = BLOCKED_RECURSO;
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

        cod_op = recibir_operacion(conexion);
        pcb_updated = recibir_pcb(conexion, logger);

        if(!dictionary_has_key(dict_recursos, nombre_recurso)) {
            pcb_updated->status = TERMINATED;
            return pcb_updated;
        } 

        t_recursos* recurso = dictionary_get(dict_recursos, nombre_recurso);

        recurso->cant_instancias += 1;

        if(!queue_is_empty(recurso->queue_waiting)) {
            t_pcb* pcb_to_ready = queue_pop(recurso->queue_waiting);

            sem_wait(&mutex_ready);
            queue_push(queue_ready, pcb_to_ready);
            sem_post(&mutex_ready);
            sem_post(&hay_ready);
        }

        pcb_updated->status = RUNNING_RECURSO;
    }

    return pcb_updated;
}
