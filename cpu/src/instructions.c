#include "main.h"

// Funcion generica para cambiar el valor de un registro.

void update_register_uint8(char* registro, void (*update_function) (uint8_t *, uint8_t), uint8_t value) {
    // sem_wait(&mutex_registros);
    (*update_function)(get_register(registro), value);
    // sem_post(&mutex_registros);
}

void update_register_uint32(char* registro, void (*update_function) (uint32_t *, uint32_t), uint32_t value) {
    // sem_wait(&mutex_registros);
    (*update_function)(get_register(registro), value);
    // sem_post(&mutex_registros);
}

void operation_register_uint8(char* destino, char* origen, cpu_operation operation){
    // sem_wait(&mutex_registros);

    uint8_t* reg_destino = (uint8_t*) get_register(destino);
    uint8_t* reg_origen = (uint8_t*) get_register(origen);

    if (sizeof(*(reg_destino)) != sizeof(*(reg_origen))) {
        // sem_post(&mutex_registros);
        return;
    }; // iria error

    switch (operation){
        case SUMA: set_registro_uint8(reg_destino, *(reg_destino) + *(reg_origen));
            break;
        
        case RESTA: set_registro_uint8(reg_destino, *(reg_destino) - *(reg_origen));
            break;
        default:
            break;
    }

    // sem_post(&mutex_registros);
}

void operation_register_uint32(char* destino, char* origen, cpu_operation operation){
    // sem_wait(&mutex_registros);

    uint32_t* reg_destino = (uint32_t*) get_register(destino);
    uint32_t* reg_origen = (uint32_t*) get_register(origen);

    if (sizeof(*(reg_destino)) != sizeof(*(reg_origen))) {
        // sem_post(&mutex_registros);
        return;
    }; // iria error

    switch (operation){
        case SUMA: set_registro_uint32(reg_destino, *(reg_destino) + *(reg_origen));
            break;
        
        case RESTA: set_registro_uint32(reg_destino, *(reg_destino) - *(reg_origen));
            break;
        default:
            break;
    }

    // sem_post(&mutex_registros);
}


bool jnz_register(char* registro, char* next_pc){
    // sem_wait(&mutex_registros);
    bool isZero;
    if (sizeof_register(registro) == sizeof(u_int8_t)){
        uint8_t* reg = (uint8_t*) get_register(registro);
        isZero = *(reg) == 0;
    } else {
        uint32_t* reg = (uint32_t*) get_register(registro);
        isZero = *(reg) == 0;
    }

    // sem_post(&mutex_registros);
    
    if (isZero) return false;

    uint32_t new_pc = atouint32(next_pc);

    update_register_uint32(PC, set_registro_uint32, new_pc);


    return true;
}

pid_status enviar_io(int kernel_fd, t_intruction_execute* decoded){
    t_paquete* paquete = crear_paquete(IO);
    
    agregar_io_paquete(paquete, decoded->operation, decoded->params[0], decoded->params[1]);
    enviar_paquete(paquete, kernel_fd);
    eliminar_paquete(paquete);

    log_info(logger, "Wait IO: %s - time: %s", decoded->params[0], decoded->params[1]);
    return BLOCKED;
}

// Funciones para updatear un registro (uno es para ++ y el otro para setear valor)

void pc_plus_plus(u_int32_t* registro, u_int32_t value){
    *(registro) += value;
}

void set_registro_uint8(uint8_t* registro, uint8_t value) {
    *(registro) = value;
}

void set_registro_uint32(uint32_t* registro, uint32_t value) {
    *(registro) = value;
}

pid_status resize_process(int memoria_fd, char* size){
    uint32_t new_size = atouint32(size);
    op_code code_op = MEM_RESIZE;


    send(memoria_fd, &code_op, sizeof(op_code), 0);
    send(memoria_fd, &pcb->pid, sizeof(uint32_t), 0);
    send(memoria_fd, &new_size, sizeof(uint32_t), 0);
    recv(memoria_fd, &code_op, sizeof(op_code), MSG_WAITALL);

    if (code_op == MEM_ERROR) return ERROR;

    return RUNNING;
}

// El numero en char que viene de lo leido en memoria, hay que pasarlo a uint que corresponda

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

// Mapper para pasar las instrucciones a sus ENUMS
set_instruction mapInstruction (char* intruction) {
    if (strcmp(intruction, "SET") == 0) return SET;
    if (strcmp(intruction, "SUM") == 0) return SUM;
    if (strcmp(intruction, "SUB") == 0) return SUB;
    if (strcmp(intruction, "JNZ") == 0) return JNZ;
    if (strcmp(intruction, "MOV_IN") == 0) return MOV_IN;
    if (strcmp(intruction, "MOV_OUT") == 0) return MOV_OUT;
    if (strcmp(intruction, "RESIZE") == 0) return RESIZE;
    if (strcmp(intruction, "COPY_STRING") == 0) return COPY_STRING;
    if (strcmp(intruction, "IO_STDIN_READ") == 0) return IO_STDIN_READ;
    if (strcmp(intruction, "IO_STDOUT_WRITE") == 0) return IO_STDOUT_WRITE;
    if (strcmp(intruction, "IO_GEN_SLEEP") == 0) return IO_GEN_SLEEP;
    if (strcmp(intruction, "EXIT") == 0) return EXIT;
    return UNKNOWN;
}

// Retorna la direccion de memoria del registro que se quiere modificar
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
