#include "main.h"


void agregar_uint_a_paquete(t_paquete* paquete, void* valor, int tamanio){

    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio);

    memcpy(paquete->buffer->stream + paquete->buffer->size, valor, tamanio);

    paquete->buffer->size += tamanio; 
}

void cpu(int conexion){
    t_list* list_instruction;
    t_intruction_execute* decoded_instruction;

    list_instruction = fetch(conexion);

    decoded_instruction = decode(list_instruction);

    exec(decoded_instruction);
}

t_list* fetch(int conexion) {
    t_list* list_instruction;
    t_paquete* pc_paquete = crear_paquete(GET_INSTRUCTION);
    u_int32_t pid = 20;

    agregar_uint_a_paquete(pc_paquete, &registros->pc, sizeof(uint32_t));
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
        
        default:
            decoded_instruction->operation = EXIT;
            break;
    }

    decoded_instruction->total_params = 2;

    return decoded_instruction;
}

void exec(t_intruction_execute* decoded_instruction) {
    switch (decoded_instruction->operation)
    {
    case SET:
        setTo(decoded_instruction->params[0], decoded_instruction->params[1]);
        break;
    
    default:
        break;
    }

    free(decoded_instruction);
}

void setTo(char* registro, char* valor){
    char *endptr; // Used to detect conversion errors

    uint8_t u_valor = (uint8_t)strtoul(valor, &endptr, 10);

    if (*endptr != '\0') {
        printf("Error: Conversion failed. Non-numeric characters found: %s\n", endptr);
        return;
    }

    uint8_t * regis = getRegister(registro);

    *(regis) = u_valor;
}

void check_interrupt() { 

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

set_instruction mapInstruction (char* intruction) {
    if (strcmp(intruction, "SET") == 0) return SET;
    if (strcmp(intruction, "SUM") == 0) return SUM;
    if (strcmp(intruction, "SUB") == 0) return SUB;
    if (strcmp(intruction, "JNZ") == 0) return JNZ;
    if (strcmp(intruction, "IO_GEN_SLEEP") == 0) return IO_GEN_SLEEP;
    return -1;
}

void* getRegister (char* registro) {    
    if (strcmp(registro, AX) == 0) return &(registros->ax);
    return registros;
}