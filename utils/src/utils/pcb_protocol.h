#ifndef UTILS_PCB_H_
#define UTILS_PCB_H_

#include "general_protocol.h"

typedef enum {
	NEW,
	RUNNING,
    TERMINATED,
    BLOCKED,
    ERROR
} pid_status; // Kernel - cpu

typedef struct {
    uint32_t pc, si, di;
    uint8_t ax, bx, cx, dx;
    uint32_t eax, ebx, ecx, edx;
} t_registros; // Kernel - cpu

typedef struct {
	uint32_t pid, pc, quantum;
	t_registros* registers;
	pid_status status;
} t_pcb; // Kernel - cpu

t_pcb* recibir_pcb(int, t_log*);
void agregar_pcb_paquete(t_paquete*, t_pcb*);

void log_registers (t_pcb*, t_log*);

#endif