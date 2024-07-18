#include "main.h"

// Funcion generica para cambiar el valor de un registro.

void update_register_uint8(char* registro, void (*update_function) (uint8_t *, uint8_t), uint8_t value) {
    (*update_function)(get_register(registro), value);
}

void update_register_uint32(char* registro, void (*update_function) (uint32_t *, uint32_t), uint32_t value) {
    (*update_function)(get_register(registro), value);
}

void operation_register_uint8(char* destino, char* origen, cpu_operation operation){
    uint8_t* reg_destino = (uint8_t*) get_register(destino);
    uint8_t* reg_origen = (uint8_t*) get_register(origen);

    if (sizeof(*(reg_destino)) != sizeof(*(reg_origen))) return;

    switch (operation){
        case SUMA: set_registro_uint8(reg_destino, *(reg_destino) + *(reg_origen));
            break;
        
        case RESTA: set_registro_uint8(reg_destino, *(reg_destino) - *(reg_origen));
            break;
        default:
            break;
    }
}

void operation_register_uint32(char* destino, char* origen, cpu_operation operation){
    uint32_t* reg_destino = (uint32_t*) get_register(destino);
    uint32_t* reg_origen = (uint32_t*) get_register(origen);

    if (sizeof(*(reg_destino)) != sizeof(*(reg_origen))) return;

    switch (operation){
        case SUMA: set_registro_uint32(reg_destino, *(reg_destino) + *(reg_origen));
            break;
        
        case RESTA: set_registro_uint32(reg_destino, *(reg_destino) - *(reg_origen));
            break;
        default:
            break;
    }
}


bool jnz_register(char* registro, char* next_pc){
    bool isZero;

    if (sizeof_register(registro) == sizeof(u_int8_t)){
        uint8_t* reg = (uint8_t*) get_register(registro);
        isZero = *(reg) == 0;
    } else {
        uint32_t* reg = (uint32_t*) get_register(registro);
        isZero = *(reg) == 0;
    }

    if (isZero) return false;

    uint32_t new_pc = atouint32(next_pc);
    update_register_uint32(PC, set_registro_uint32, new_pc);

    return true;
}

pid_status enviar_io(int kernel_fd, t_intruction_execute* decoded){
    t_paquete* paquete = crear_paquete(IO);
    
    agregar_io_paquete(paquete, decoded->operation, decoded->params, decoded->total_params);
    enviar_paquete(paquete, kernel_fd);
    eliminar_paquete(paquete);

    return BLOCKED_IO;
}

pid_status enviar_io_truncate_fs(int kernel_fd, t_intruction_execute* decoded){
    uint32_t tamaño_register = get_normaliced_register(decoded->params[2]);

    t_paquete* paquete = crear_paquete(IO);
    uint32_t buffer_size = sizeof(uint32_t) + strlen(decoded->params[1]) + 1 + sizeof(int);

    agregar_io_paquete(paquete, decoded->operation, decoded->params, 1);
    agregar_uint_a_paquete(paquete, &buffer_size, sizeof(uint32_t));
    agregar_a_paquete(paquete, decoded->params[1], strlen(decoded->params[1]) + 1);
    agregar_uint_a_paquete(paquete, &tamaño_register, sizeof(uint32_t));

    enviar_paquete(paquete, kernel_fd);
    eliminar_paquete(paquete);

    return BLOCKED_IO;
}

pid_status mov_out(int memoria_fd, char* registro_dl, char* registro_datos){
    uint32_t size_register = (uint32_t) sizeof_register(registro_datos);
    uint32_t direccion_logica = get_normaliced_register(registro_dl);

    void* buffer = malloc(size_register);
    t_list* frames = list_create();
    pid_status status;

    if (size_register == sizeof(uint32_t)) memcpy(buffer, (uint32_t*) get_register(registro_datos), size_register);
    else memcpy(buffer, (uint8_t*) get_register(registro_datos), size_register);

    status = mmu(memoria_fd, direccion_logica, size_register, frames);

    if (status == RUNNING) {
        int error = escribir_memoria(memoria_fd, buffer, frames);
        status = error == -1 ? ERROR : RUNNING;
    }
    
    list_destroy(frames);
    free(buffer);
    return status;
}

pid_status mov_in(int memoria_fd, char* registro_datos, char* registro_dl){
    uint32_t size_register = (uint32_t) sizeof_register(registro_datos);
    uint32_t direccion_logica = get_normaliced_register(registro_dl);

    void* buffer = malloc(size_register);
    t_list* frames = list_create();
    pid_status status;

    status = mmu(memoria_fd, direccion_logica, size_register, frames);

    if (status == RUNNING) {
        int error = leer_memoria(memoria_fd, buffer, frames);
        status = error == -1 ? ERROR : RUNNING;
    }

    if (size_register == sizeof(uint32_t)) memcpy((uint32_t*) get_register(registro_datos), buffer, size_register);
    else memcpy((uint8_t*) get_register(registro_datos), buffer, size_register);

    list_destroy(frames);
    free(buffer);
    return status;
}

pid_status copy_string(int memoria_fd, char* tamaño){
    uint32_t size = atouint32(tamaño);
    uint32_t src_register = *((uint32_t*) get_register(SI));
    uint32_t dst_register = *((uint32_t*) get_register(DI));
    t_list* src_frames = list_create();
    t_list* dst_frames = list_create();
    void* buffer = malloc(size);
    pid_status status;

    status = mmu(memoria_fd, src_register, size, src_frames);
    status = mmu(memoria_fd, dst_register, size, dst_frames);

    if (status == RUNNING) {
        int error = leer_memoria(memoria_fd, buffer, src_frames);
        status = error == -1 ? ERROR : RUNNING;
        
        if (status != ERROR){
            error = escribir_memoria(memoria_fd, buffer, dst_frames);
            status = error == -1 ? ERROR : RUNNING;
        }
    }


    list_destroy(src_frames);
    list_destroy(dst_frames);
    free(buffer);
    return status;
}

pid_status io_read_write(int memoria_fd, int kernel_fd, t_intruction_execute* decoded){
    uint32_t tamaño_register = get_normaliced_register(decoded->params[2]);
    uint32_t dl_register = get_normaliced_register(decoded->params[1]);
    pid_status status;

    t_list* frames = list_create();
    status = mmu(memoria_fd, dl_register, tamaño_register, frames);

    if (status == RUNNING) {
        t_paquete* paquete = crear_paquete(IO);
        uint32_t buffer_size = list_size(frames) * sizeof(t_memoria_fisica)  + sizeof(uint32_t); // tamaño de lo que hay que leer mas tamaño de los frames (cada frame 8 bytes) -> Para que coincida conIO_GES_SLEEP PARA KERNEL

        agregar_io_paquete(paquete, decoded->operation, decoded->params, 1); // Solo necesito agregar el nombre como si fuese IO_GEN_SLEEP
        agregar_uint_a_paquete(paquete, &buffer_size, sizeof(uint32_t));
        agregar_uint_a_paquete(paquete, &tamaño_register, sizeof(uint32_t));
        
        while(!list_is_empty(frames)){
            t_memoria_fisica* frame = list_remove(frames, 0);
            
            agregar_uint_a_paquete(paquete, &frame->direccion_fisica, sizeof(uint32_t));
            agregar_uint_a_paquete(paquete, &frame->bytes, sizeof(uint32_t));

            free(frame);
        }

        enviar_paquete(paquete, kernel_fd);
        eliminar_paquete(paquete);

        return BLOCKED_IO;
    }

    return status;
}

pid_status io_read_write_fs(int memoria_fd, int kernel_fd, t_intruction_execute* decoded){
    uint32_t tamaño_register = get_normaliced_register(decoded->params[3]);
    uint32_t dl_register = get_normaliced_register(decoded->params[2]);
    uint32_t ptr_archivo = get_normaliced_register(decoded->params[4]);
    pid_status status;

    t_list* frames = list_create();
    status = mmu(memoria_fd, dl_register, tamaño_register, frames);

    if (status == RUNNING) {
        t_paquete* paquete = crear_paquete(IO);
        /* 
            Como Kernel solo hace forward, necesita de ante mano saber todo el buffer. Este esta compuesto por:
                estructura de frame * cantidad de frames +
                strlen(nombre_archivo) + 1 + int (agregar a paquete agregar el tmaaño de la cadena)
                2 * uint32 por el registro tamaño y registro puntero de archivo.
        */
        uint32_t buffer_size = list_size(frames) * sizeof(t_memoria_fisica)  + 2 * sizeof(uint32_t) + strlen(decoded->params[1]) + 1 + sizeof(int);

        agregar_io_paquete(paquete, decoded->operation, decoded->params, 1); // Solo necesito agregar el nombre como si fuese IO_GEN_SLEEP
        agregar_uint_a_paquete(paquete, &buffer_size, sizeof(uint32_t));
        agregar_a_paquete(paquete, decoded->params[1], strlen(decoded->params[1]) + 1);
        agregar_uint_a_paquete(paquete, &ptr_archivo, sizeof(uint32_t));
        agregar_uint_a_paquete(paquete, &tamaño_register, sizeof(uint32_t));
        
        while(!list_is_empty(frames)){
            t_memoria_fisica* frame = list_remove(frames, 0);
            
            agregar_uint_a_paquete(paquete, &frame->direccion_fisica, sizeof(uint32_t));
            agregar_uint_a_paquete(paquete, &frame->bytes, sizeof(uint32_t));

            free(frame);
        }

        enviar_paquete(paquete, kernel_fd);
        eliminar_paquete(paquete);

        return BLOCKED_IO;
    }

    return status;
}

void semaphore(int kernek_fd, op_code code, char* recurso){
    enviar_mensaje(recurso, kernek_fd, code);
}

pid_status resize_process(int memoria_fd, char* size){
    uint32_t new_size = atouint32(size);
    t_paquete* paquete = crear_paquete(MEM_RESIZE);

    agregar_uint_a_paquete(paquete, &pcb->pid, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &new_size, sizeof(uint32_t));
    enviar_paquete(paquete, memoria_fd);
    eliminar_paquete(paquete);

    op_code code_op;
    recv(memoria_fd, &code_op, sizeof(op_code), MSG_WAITALL);

    if (code_op == MEM_ERROR) return ERROR;

    return RUNNING;
}

