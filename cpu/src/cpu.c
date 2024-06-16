#include "main.h"

void cpu(int memoria_fd, int kernel_fd){
    t_list* list_instruction;
    t_intruction_execute* decoded_instruction;
    pid_status result = pcb->status;

    while(result == RUNNING) {
        list_instruction = fetch(memoria_fd);
        decoded_instruction = decode(list_instruction);
        result = exec(decoded_instruction, kernel_fd, memoria_fd);
        bool interrupt = check_interrupt(result);
        if (interrupt && result == RUNNING) result = RUNNING_QUANTUM; 
    }

    pcb->pc = pcb->registers->pc;
    pcb->status = result;

    t_paquete* paquete = crear_paquete(PCB);

    agregar_pcb_paquete(paquete, pcb);
    enviar_paquete(paquete, kernel_fd);
    eliminar_paquete(paquete);

    log_info(logger, "Proceso enviado a Kernel - PID: %u", pcb->pid);
    free(pcb->registers);
    free(pcb);
}

t_list* fetch(int memoria_fd) {
    t_list* list_instruction;
    t_paquete* pc_paquete = crear_paquete(INSTRUCTION);

    agregar_uint_a_paquete(pc_paquete, get_register(PC), sizeof(uint32_t));
    agregar_uint_a_paquete(pc_paquete, &pcb->pid, sizeof(uint32_t));

    enviar_paquete(pc_paquete, memoria_fd);
    eliminar_paquete(pc_paquete);

    log_info(logger, "Fetch PC: %u", pcb->registers->pc);
    
    recibir_operacion(memoria_fd);
    list_instruction = recibir_paquete(memoria_fd);

    return list_instruction;
}

t_intruction_execute* decode(t_list* list_instruction) {
    t_intruction_execute* decoded_instruction = malloc(sizeof(t_intruction_execute));
    char* params = string_new();

    void paramToPrint(void* param) {
        string_append(&params, (char*) param);
        string_append(&params, " ");
    }

    char* instruction_type = list_remove(list_instruction, 0);
    list_iterate(list_instruction, &paramToPrint);
    log_info(logger, "PID: %u - Ejectuando: %s - %s", pcb->pid, instruction_type, params);
    
    switch (mapInstruction(instruction_type)){
        case SET:
            decoded_instruction->operation = SET;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            decoded_instruction->total_params = 2;
            break;
        case SUM:
            decoded_instruction->operation = SUM;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            decoded_instruction->total_params = 2;
            break;
        case SUB:
            decoded_instruction->operation = SUB; 
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            decoded_instruction->total_params = 2;
            break;
        case JNZ:
            decoded_instruction->operation = JNZ;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            decoded_instruction->total_params = 2;
            break;

        case RESIZE:
            decoded_instruction->operation = RESIZE;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->total_params = 1;
            break;

        case MOV_IN:
            decoded_instruction->operation = MOV_IN;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            decoded_instruction->total_params = 2;
            break;

        case MOV_OUT:
            decoded_instruction->operation = MOV_OUT;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            decoded_instruction->total_params = 2;
            break;
        
        case COPY_STRING:
            decoded_instruction->operation = COPY_STRING;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->total_params = 1;
            break;
        
        case WAIT: 
            decoded_instruction->operation = WAIT;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->total_params = 1;
            break;

        case SIGNAL: 
            decoded_instruction->operation = SIGNAL;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->total_params = 1;
            break;

        case IO_GEN_SLEEP:
            decoded_instruction->operation = IO_GEN_SLEEP;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            decoded_instruction->total_params = 2;
            break;    
        
        case IO_STDIN_READ:
            decoded_instruction->operation = IO_STDIN_READ;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            decoded_instruction->params[2] = list_remove(list_instruction, 0);
            decoded_instruction->total_params = 3;
            break;   
        
        case IO_STDOUT_WRITE:
            decoded_instruction->operation = IO_STDOUT_WRITE;
            decoded_instruction->params[0] = list_remove(list_instruction, 0);
            decoded_instruction->params[1] = list_remove(list_instruction, 0);
            decoded_instruction->params[2] = list_remove(list_instruction, 0);
            decoded_instruction->total_params = 3;
            break;   

        case EXIT:
            decoded_instruction->operation = EXIT;
            decoded_instruction->total_params = 0;
            break;
        
        default:
            decoded_instruction->operation = UNKNOWN;
            break;
    }

    list_destroy(list_instruction);

    return decoded_instruction;
}

pid_status exec(t_intruction_execute* decoded_instruction, int kernel_fd, int memoria_fd) {
    pid_status status = RUNNING; 
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

        case RESIZE: 
            status = resize_process(memoria_fd, decoded_instruction->params[0]);
            break;

        case MOV_IN:
            status = mov_in(memoria_fd, decoded_instruction->params[0], decoded_instruction->params[1]);
            break;

        case MOV_OUT:
            status = mov_out(memoria_fd, decoded_instruction->params[0], decoded_instruction->params[1]);
            break;

        case COPY_STRING:
            status = copy_string(memoria_fd, decoded_instruction->params[0]);
            break;

        case WAIT:
            semaphore(kernel_fd, WAIT_RECURSO,  decoded_instruction->params[0]);
            status = BLOCKED_WAIT;
            break;

        case SIGNAL:
            semaphore(kernel_fd, SIGNAL_RECURSO,  decoded_instruction->params[0]);
            status = RUNNING_SIGNAL;
            break;

        case IO_GEN_SLEEP:
            status = enviar_io(kernel_fd, decoded_instruction);
            break;

        case IO_STDIN_READ:
            status = io_read_write(memoria_fd, kernel_fd, decoded_instruction);
            break;

        case IO_STDOUT_WRITE:
            status = io_read_write(memoria_fd, kernel_fd, decoded_instruction);
            break;

        case EXIT: 
            status = TERMINATED;
            break;

        default:
            status = ERROR;
            break;
    }

    free(decoded_instruction);

    if (!ignorePC) update_register_uint32(PC, pc_plus_plus, (uint32_t) 1);

    return status;
}

bool check_interrupt() { 
    bool interrupt;
    sem_wait(&mutex_interrupt);
    interrupt = has_interrupt;
    has_interrupt = false;
    sem_post(&mutex_interrupt);

    return interrupt;
}
