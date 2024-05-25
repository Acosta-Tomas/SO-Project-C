#include "main.h"

void* corto_main(void *arg){
    log_info(logger, "Thread planificado de corto plazo creado");

    char* ip_cpu = config_get_string_value(config, KEY_IP_CPU);
    char* puerto_cpu_dispatch = config_get_string_value(config, KEY_PUERTO_CPU_DISPATCH);
    int conexion = crear_conexion(ip_cpu, puerto_cpu_dispatch);

    log_info(logger, "Connected to CPU - SOCKET: %d", conexion);

    while (1){
        sem_wait(&hay_ready);
        sem_wait(&mutex_ready);
        t_pcb* pcb = queue_pop(queue_ready);
        pcb->status = RUNNING;
        sem_post(&mutex_ready);

        enviar_cpu(conexion, pcb);
        pcb = esperar_cpu(conexion);

        if (pcb->status == TERMINATED || pcb->status == UNKNOWN) {
            log_info(logger, "Proceso Finalizado - PID: %u", pcb->pid);
            log_registers(pcb, logger);
            free(pcb->registers);
            free(pcb);
        }

        if (pcb->status == ERROR) {
            log_error(logger, "Error en proceso - PID: %u", pcb->pid);
            free(pcb->registers);
            free(pcb);
        }

        if (pcb->status == RUNNING) {
            log_info(logger, "Procesi fin de Quantum - PID: %u", pcb->pid);

            sem_wait(&mutex_ready);
            queue_push(queue_ready, pcb);
            sem_post(&mutex_ready);
            sem_post(&hay_ready);
        }
    }
    
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
    int cod_op = recibir_operacion(conexion);

    if (cod_op == PCB) {
        pcb_updated = recibir_pcb(conexion, logger);
    }

    return pcb_updated;
}
