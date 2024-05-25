#include <utils/general.h>

void decir_hola(char* quien) {
    printf("Hola desde %s!!\n", quien);
}

void leer_consola(t_log* logger){
	char* leido;

	for (leido = readline("> "); leido && strcmp(leido, ""); leido = readline("> ")){
		log_info(logger, "%s", leido);
		free(leido);
	}
	
	if (leido != NULL) free(leido);

	return;
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