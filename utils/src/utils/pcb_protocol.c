#include "pcb_protocol.h"

t_pcb* recibir_pcb(int conexion, t_log* logger){
    int size;
    int desplazamiento = 0;
    uint32_t size_recursos;
    void* buffer;
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->registers = malloc(sizeof(t_registros));

    buffer = recibir_buffer(&size, conexion);

    memcpy(&pcb->pid, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->pc, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->quantum, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->status, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->ax, buffer + desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&pcb->registers->bx, buffer + desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&pcb->registers->cx, buffer + desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&pcb->registers->dx, buffer + desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&pcb->registers->eax, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->ebx, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->ecx, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->edx, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->si, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->di, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->pc, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&size_recursos, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    pcb->recursos = malloc(size_recursos);
    memcpy(pcb->recursos, buffer + desplazamiento, size_recursos);
    desplazamiento += size_recursos;

    if (size != desplazamiento) log_info(logger, "Error al recibir PCB");

	free(buffer);

    return pcb;
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

    agregar_a_paquete(paquete, pcb->recursos, strlen(pcb->recursos) + 1);
}

void log_registers (t_pcb* pcb, t_log* logger) {
    log_info(logger, "PC %u", pcb->registers->pc);
    log_info(logger, "AX %hhu", pcb->registers->ax);
    log_info(logger, "BX %hhu", pcb->registers->bx);
    log_info(logger, "CX %hhu", pcb->registers->cx);
    log_info(logger, "DX %hhu", pcb->registers->dx);
    log_info(logger, "EAX %u", pcb->registers->eax);
    log_info(logger, "EBX %u", pcb->registers->ebx);
    log_info(logger, "ECX %u", pcb->registers->ecx);
    log_info(logger, "EDX %u", pcb->registers->edx);
    log_info(logger, "SI %u", pcb->registers->si);
    log_info(logger, "DI %u", pcb->registers->di);
}