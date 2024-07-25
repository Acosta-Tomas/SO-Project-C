#include "instruction_protocol.h"

int escribir_memoria(int memoria_fd, void* buffer, t_list* frames, t_log* logger, uint32_t pid){
    int offset_buffer = 0;

    while(list_size(frames)){
        t_memoria_fisica* frame = list_remove(frames, 0);
        t_paquete* paquete = crear_paquete(MEM_WRITE);

        agregar_uint_a_paquete(paquete, &frame->direccion_fisica, sizeof(uint32_t));
        agregar_a_paquete(paquete, buffer + offset_buffer, frame->bytes);

        enviar_paquete(paquete, memoria_fd);
        eliminar_paquete(paquete);

        char* value_print = malloc(frame->bytes + 1);
        memcpy(value_print, buffer + offset_buffer, frame->bytes);
        log_info(logger, "PID: %u - Acción: ESCRIBIR - Dirección Física: %u - Valor: %s", pid, frame->direccion_fisica, value_print);


        offset_buffer += frame->bytes;
        free(frame);
        free(value_print);

        op_code code;
        recv(memoria_fd, &code, sizeof(op_code), MSG_WAITALL);

        if (code != MEM_SUCCESS) return -1;
    }

    return 0;
}

int leer_memoria(int memoria_fd, void* buffer, t_list* frames, t_log* logger, uint32_t pid){
    int offset_buffer = 0;

    while(list_size(frames)){
        t_memoria_fisica* frame = list_remove(frames, 0);
        t_paquete* paquete = crear_paquete(MEM_READ);

        agregar_uint_a_paquete(paquete, &frame->direccion_fisica, sizeof(uint32_t));
        agregar_uint_a_paquete(paquete, &frame->bytes, sizeof(uint32_t));

        enviar_paquete(paquete, memoria_fd);
        eliminar_paquete(paquete);

        int buffer_size;
        op_code code = recibir_operacion(memoria_fd);
        void* buffer_memoria = recibir_buffer(&buffer_size, memoria_fd);

        memcpy(&frame->bytes, buffer_memoria, sizeof(uint32_t));
        char* value_print = malloc(frame->bytes + 1);
        memcpy(buffer + offset_buffer, buffer_memoria + sizeof(uint32_t), frame->bytes);
        memcpy(value_print, buffer_memoria + sizeof(uint32_t), frame->bytes);

        log_info(logger, "PID: %u - Acción: LEER - Dirección Física: %u - Valor: %s", pid, frame->direccion_fisica, value_print);

        offset_buffer += frame->bytes;
        free(frame);
        free(buffer_memoria);
        free(value_print);

        if (code != MEM_SUCCESS) return -1;
    }

    return 0;
}