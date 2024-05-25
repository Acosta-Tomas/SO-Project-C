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

        if (pcb->status == TERMINATED) {
            log_info(logger, "Proceso Finalizado - PID: %u", pcb->pid);
            free(pcb->registers);
            free(pcb_updated);
        }

        if (pcb->status == ERROR) {
            log_error(logger, "Error en proceso - PID: %u", pcb->pid);
            free(pcb->registers);
            free(pcb_updated);
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

    enviar_paquete(paquete, conexion)

    eliminar_paquete(paquete);

    log_info(logger, "Proceso enviado a CPU - PID: %u", pcb->pid);
    free(pcb->registers);
    free(pcb);
}

void agregar_pcb_paquete(t_paquete* paquete, t_pcb* pcb){
    agregar_uint_a_paquete(paquete, &pcb->pid, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->pc, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->quantum, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->status, sizeof(pid_status));

    // registros
    agregar_uint_a_paquete(paquete, &pcb->registers->ax, sizeof(uint8_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->bx, sizeof(uint8_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->cx, sizeof(uint8_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->dx, sizeof(uint8_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->eax, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->ebx, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->ecx, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->edx, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->si, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->di, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &pcb->registers->pc, sizeof(uint32_t));
}

t_pcb* esperar_cpu(int conexion){
    t_pcb* pcb_updated;
    int cod_op = recibir_operacion(conexion);

    if (cod_op == PCB) {
        pcb_updated = recibir_pcb(conexion);
    }

    return pcb_updated;
}

t_pcb* recibir_pcb(int conexion){
    int size;
    int desplazamiento = 0;
    void* buffer;
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->registers = malloc(sizeof(t_registros));

    buffer = recibir_buffer(&size, conexion);

    memcpy(&pcb->pid, buffer - desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->pc, buffer - desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->quantum, buffer - desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->status, buffer - desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->ax, buffer - desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&pcb->registers->bx, buffer - desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&pcb->registers->cx, buffer - desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&pcb->registers->dx, buffer - desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&pcb->registers->eax, buffer - desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->ebx, buffer - desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->ecx, buffer - desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->edx, buffer - desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->si, buffer - desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->di, buffer - desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->pc, buffer - desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    if (size != 0) log_info(logger, "Error al recibir PCB");

	free(buffer);

    return pcb
}