#include "instruction_protocol.h"

int escribir_memoria(int memoria_fd, void* buffer, t_list* frames){
    int offset_buffer = 0;

    while(list_size(frames)){
        t_memoria_fisica* frame = list_remove(frames, 0);
        t_paquete* paquete = crear_paquete(MEM_WRITE);

        agregar_uint_a_paquete(paquete, &frame->direccion_fisica, sizeof(uint32_t));
        agregar_a_paquete(paquete, buffer + offset_buffer, frame->bytes);

        enviar_paquete(paquete, memoria_fd);
        eliminar_paquete(paquete);

        offset_buffer += frame->bytes;
        free(frame);

        op_code code;
        recv(memoria_fd, &code, sizeof(op_code), MSG_WAITALL);

        if (code != MEM_SUCCESS) return -1;
    }

    return 0;
}

int leer_memoria(int memoria_fd, void* buffer, t_list* frames){
    int offset_buffer = 0;

    while(list_size(frames)){
        t_memoria_fisica* frame = list_remove(frames, 0);
        t_paquete* paquete = crear_paquete(MEM_READ);

        agregar_uint_a_paquete(paquete, &frame->direccion_fisica, sizeof(uint32_t));
        agregar_uint_a_paquete(paquete, &frame->bytes, sizeof(uint32_t));

        enviar_paquete(paquete, memoria_fd);
        eliminar_paquete(paquete);

        op_code code;
        recv(memoria_fd, &code, sizeof(op_code), MSG_WAITALL);
        recv(memoria_fd, &frame->bytes, sizeof(uint32_t), MSG_WAITALL);
        recv(memoria_fd, buffer + offset_buffer, frame->bytes, MSG_WAITALL);

        offset_buffer += frame->bytes;
        free(frame);

        if (code != MEM_SUCCESS) return -1;
    }

    return 0;
}