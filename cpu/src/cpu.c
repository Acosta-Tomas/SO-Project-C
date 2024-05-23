#include "main.h"

void cpu(int conexion){
    t_list* list_instruction;
    t_intruction_execute* decoded_instruction;
    cpu_status result;

    while(1) {
        sem_wait(&run);
        result = RUNNING;

        while(result == RUNNING) {
            list_instruction = fetch(conexion);
            decoded_instruction = decode(list_instruction);
            result = exec(decoded_instruction);
        }
        sem_post(&wait);
    }

}

t_list* fetch(int conexion) {
    t_list* list_instruction;
    t_paquete* pc_paquete = crear_paquete(GET_INSTRUCTION);
    u_int32_t pid = 20;

    agregar_uint_a_paquete(pc_paquete, get_register(PC), sizeof(uint32_t));
    agregar_uint_a_paquete(pc_paquete, &pid, sizeof(uint32_t));

    enviar_paquete(pc_paquete, conexion);
    
    eliminar_paquete(pc_paquete);
    
    int cod_op = recibir_operacion(conexion);
    
    log_info(logger, "Codigo Operacion %i", cod_op);

    list_instruction = recibir_paquete(conexion);

    return list_instruction;
}

t_intruction_execute* decode(t_list* list_instruction) {
    t_intruction_execute* decoded_instruction = malloc(sizeof(t_intruction_execute));
    
    char* instruction_type = list_remove(list_instruction, 0);

    switch (mapInstruction(instruction_type)){
        case SET:
            decoded_instruction->operation = SET;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            break;
        case SUM:
            decoded_instruction->operation = SUM;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            break;
        case SUB:
            decoded_instruction->operation = SUB; 
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            break;
        case JNZ:
            decoded_instruction->operation = JNZ;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            break;
        case IO_GEN_SLEEP:
            decoded_instruction->operation = IO_GEN_SLEEP;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            break;    

        case EXIT:
            decoded_instruction->operation = EXIT;
            break;
        
        default:
            decoded_instruction->operation = UNKNOWN;
            break;
    }

    decoded_instruction->total_params = 2;

    list_destroy(list_instruction);

    return decoded_instruction;
}

cpu_status exec(t_intruction_execute* decoded_instruction) {
    cpu_status status = RUNNING; 
    bool ignorePC = false;

    switch (decoded_instruction->operation){
        case SET: 
            if (sizeof(uint8_t) == sizeof_register(decoded_instruction->params[0]))
                update_register_uint8(decoded_instruction->params[0], set_registro_uint8, atouint8(decoded_instruction->params[1]));
            else 
                update_register_uint32(decoded_instruction->params[0], set_registro_uint32, atouint32(decoded_instruction->params[1]));
            break;

        case SUM:
            if (sizeof(uint8_t) == sizeof_register(decoded_instruction->params[0]))
                operation_register_uint8(decoded_instruction->params[0], decoded_instruction->params[1], SUMA);
            else 
                operation_register_uint32(decoded_instruction->params[0], decoded_instruction->params[1], SUMA);
            break;

        case SUB:
            if (sizeof(uint8_t) == sizeof_register(decoded_instruction->params[0]))
                operation_register_uint8(decoded_instruction->params[0], decoded_instruction->params[1], RESTA);
            else 
                operation_register_uint32(decoded_instruction->params[0], decoded_instruction->params[1], RESTA);
            break;

        case JNZ:
            ignorePC = jnz_register(decoded_instruction->params[0], decoded_instruction->params[1]);
            break;
        case EXIT: 
            status = TERMINATED;
            break;

        default:
            status = UNKNOWN;
            break;
    }

    free(decoded_instruction);

    if (!ignorePC) update_register_uint32(PC, pc_plus_plus, (uint32_t) 1);

    return status;
}

void check_interrupt() { 

}

// Funcion generica para cambiar el valor de un registro.

void update_register_uint8(char* registro, void (*update_function) (uint8_t *, uint8_t), uint8_t value) {
    sem_wait(&mutex_registros);
    (*update_function)(get_register(registro), value);
    sem_post(&mutex_registros);
}

void update_register_uint32(char* registro, void (*update_function) (uint32_t *, uint32_t), uint32_t value) {
    sem_wait(&mutex_registros);
    (*update_function)(get_register(registro), value);
    sem_post(&mutex_registros);
}

void operation_register_uint8(char* destino, char* origen, cpu_operation operation){
    sem_wait(&mutex_registros);

    uint8_t* reg_destino = (uint8_t*) get_register(destino);
    uint8_t* reg_origen = (uint8_t*) get_register(origen);

    if (sizeof(*(reg_destino)) != sizeof(*(reg_origen))) {
        sem_post(&mutex_registros);
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

    sem_post(&mutex_registros);
}

void operation_register_uint32(char* destino, char* origen, cpu_operation operation){
    sem_wait(&mutex_registros);

    uint32_t* reg_destino = (uint32_t*) get_register(destino);
    uint32_t* reg_origen = (uint32_t*) get_register(origen);

    if (sizeof(*(reg_destino)) != sizeof(*(reg_origen))) {
        sem_post(&mutex_registros);
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

    sem_post(&mutex_registros);
}


bool jnz_register(char* registro, char* next_pc){
    sem_wait(&mutex_registros);
    uint8_t* reg = (uint8_t*) get_register(registro);
    bool isZero = *(reg) == 0;
    sem_post(&mutex_registros);
    
    if (isZero) return false;

    uint32_t new_pc = atouint32(next_pc);

    update_register_uint32(PC, set_registro_uint32, new_pc);


    return true;
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
    if (strcmp(intruction, "IO_GEN_SLEEP") == 0) return IO_GEN_SLEEP;
    if (strcmp(intruction, "EXIT") == 0) return EXIT;
    return UNKNOWN;
}

// Retorna la direccion de memoria del registro que se quiere modificar
void* get_register (char* registro) {    
    if (strcmp(registro, PC) == 0) return &(registros->pc);
    if (strcmp(registro, AX) == 0) return &(registros->ax);
    if (strcmp(registro, BX) == 0) return &(registros->bx);
    if (strcmp(registro, CX) == 0) return &(registros->cx);
    if (strcmp(registro, DX) == 0) return &(registros->di);
    if (strcmp(registro, EAX) == 0) return &(registros->eax);
    if (strcmp(registro, EBX) == 0) return &(registros->ebx);
    if (strcmp(registro, ECX) == 0) return &(registros->ecx);
    if (strcmp(registro, EDX) == 0) return &(registros->edx);
    if (strcmp(registro, SI) == 0) return &(registros->si);
    if (strcmp(registro, DI) == 0) return &(registros->di);
    return registros;
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

t_registros *create_registros() {
	t_registros* regis = malloc(sizeof(regis));

    regis->pc = 0;

    regis->ax = 0;
    regis->bx = 0;
    regis->cx = 0;
    regis->dx = 0;

    regis->eax = 0;
    regis->ebx = 0;
    regis->ecx = 0;
    regis->edx = 0;

    regis->si = 0;
    regis->di = 0;
    
    return regis;
}

void log_registers () {
    log_info(logger, "PC %u", registros->pc);
    log_info(logger, "AX %hhu", registros->ax);
    log_info(logger, "BC %hhu", registros->bx);
    log_info(logger, "CX %hhu", registros->cx);
    log_info(logger, "DX %hhu", registros->dx);
    log_info(logger, "EAX %u", registros->eax);
    log_info(logger, "EBX %u", registros->ebx);
    log_info(logger, "ECX %u", registros->ecx);
    log_info(logger, "EDX %u", registros->edx);
    log_info(logger, "SI %u", registros->si);
    log_info(logger, "DI %u", registros->di);
}