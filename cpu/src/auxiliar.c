#include "main.h"

void pc_plus_plus(u_int32_t* registro, u_int32_t value){
    *(registro) += value;
}

void set_registro_uint8(uint8_t* registro, uint8_t value) {
    *(registro) = value;
}

void set_registro_uint32(uint32_t* registro, uint32_t value) {
    *(registro) = value;
}

uint32_t get_normaliced_register(char* regis){
    uint32_t size_register = (uint32_t) sizeof_register(regis);
    uint32_t norm_register = sizeof(uint32_t) == size_register ? *((uint32_t*) get_register(regis)) : (uint32_t) *((uint8_t*) get_register(regis));

    return norm_register;
}

uint8_t atouint8(char* value) {
    char *endptr; // Used to detect conversion errors
    uint8_t new_value = (uint8_t) strtoul(value, &endptr, 10);

    if (*endptr != '\0') {
        printf("Error: Conversion failed. Non-numeric characters found: %s\n", endptr);
        return 0;
    }
    
    return new_value;
}

uint32_t atouint32(char* value) {
    char *endptr; // Used to detect conversion errors
    uint32_t new_value = (uint32_t) strtoul(value, &endptr, 10);

    if (*endptr != '\0') {
        printf("Error: Conversion failed. Non-numeric characters found: %s\n", endptr);
        return 0;
    }
    
    return new_value;
}

set_instruction mapInstruction (char* intruction) {
    if (strcmp(intruction, "SET") == 0) return SET;
    if (strcmp(intruction, "SUM") == 0) return SUM;
    if (strcmp(intruction, "SUB") == 0) return SUB;
    if (strcmp(intruction, "JNZ") == 0) return JNZ;
    if (strcmp(intruction, "MOV_IN") == 0) return MOV_IN;
    if (strcmp(intruction, "MOV_OUT") == 0) return MOV_OUT;
    if (strcmp(intruction, "RESIZE") == 0) return RESIZE;
    if (strcmp(intruction, "COPY_STRING") == 0) return COPY_STRING;
    if (strcmp(intruction, "WAIT") == 0) return WAIT;
    if (strcmp(intruction, "SIGNAL") == 0) return SIGNAL;
    if (strcmp(intruction, "IO_STDIN_READ") == 0) return IO_STDIN_READ;
    if (strcmp(intruction, "IO_STDOUT_WRITE") == 0) return IO_STDOUT_WRITE;
    if (strcmp(intruction, "IO_GEN_SLEEP") == 0) return IO_GEN_SLEEP;
    if (strcmp(intruction, "IO_FS_CREATE") == 0) return IO_FS_CREATE;
    if (strcmp(intruction, "IO_FS_DELETE") == 0) return IO_FS_DELETE;
    if (strcmp(intruction, "IO_FS_TRUNCATE") == 0) return IO_FS_TRUNCATE;
    if (strcmp(intruction, "IO_FS_READ") == 0) return IO_FS_READ;
    if (strcmp(intruction, "IO_FS_WRITE") == 0) return IO_FS_WRITE;
    if (strcmp(intruction, "EXIT") == 0) return EXIT;
    return UNKNOWN;
}

void* get_register (char* registro) {    
    if (strcmp(registro, PC) == 0) return &(pcb->registers->pc);
    if (strcmp(registro, AX) == 0) return &(pcb->registers->ax);
    if (strcmp(registro, BX) == 0) return &(pcb->registers->bx);
    if (strcmp(registro, CX) == 0) return &(pcb->registers->cx);
    if (strcmp(registro, DX) == 0) return &(pcb->registers->dx);
    if (strcmp(registro, EAX) == 0) return &(pcb->registers->eax);
    if (strcmp(registro, EBX) == 0) return &(pcb->registers->ebx);
    if (strcmp(registro, ECX) == 0) return &(pcb->registers->ecx);
    if (strcmp(registro, EDX) == 0) return &(pcb->registers->edx);
    if (strcmp(registro, SI) == 0) return &(pcb->registers->si);
    if (strcmp(registro, DI) == 0) return &(pcb->registers->di);
    return pcb->registers;
}

unsigned long sizeof_register(char* registro){
    char* char_registers[4] = { AX, BX, CX, DX };
    unsigned long size = 4;

    for(int i = 0; i < 4; i += 1){
        if (strcmp(char_registers[i], registro) == 0) {
            size = 1;
            break;
        }
    }

    return size;
}

void print_instruction(t_list* list, char* instruction) {
    char* params = string_new();

    void paramToPrint(void* param) {
        string_append(&params, (char*) param);
        string_append(&params, " ");
    };

    list_iterate(list, &paramToPrint);
    log_info(logger, "PID: %u - Ejectuando: %s - %s", pcb->pid, instruction, params);
        
    free(params);
}