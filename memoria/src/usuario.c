#include "main.h"

void escribir_memoria(int client_fd){
    uint32_t direccion_fisica;
    int value_size;

    void* value_write = recibir_mem_write(client_fd, &direccion_fisica, &value_size, logger);

    sem_wait(&mutex_mem_usuario);
    memcpy(memoria_usuario + direccion_fisica, value_write, value_size);
    sem_post(&mutex_mem_usuario);

    retardo();
    op_code code_op = MEM_SUCCESS;
    send(client_fd, &code_op, sizeof(op_code), 0);

    free(value_write);
}

void leer_memoria(int client_fd){
    uint32_t direccion_fisica;
    int value_size;

    void* value_read = recibir_mem_read(client_fd, &direccion_fisica, &value_size, logger);

    sem_wait(&mutex_mem_usuario);
    memcpy(value_read, memoria_usuario + direccion_fisica, value_size);
    sem_post(&mutex_mem_usuario);
    
    retardo();
    t_paquete* paquete = crear_paquete(MEM_SUCCESS);

    agregar_a_paquete(paquete, value_read, value_size);
    enviar_paquete(paquete, client_fd);
    eliminar_paquete(paquete);

    free(value_read);
}

void resize_process(int client_fd){
    uint32_t pid, new_size;
    t_memoria* pid_mem;
    op_code code_op = MEM_SUCCESS;

    pid = recibir_pid_con_uint32(client_fd, &new_size, logger);
    pid_mem = get_pid_mem(pid);

    int pages = ceil((double)new_size / page_size) - list_size(pid_mem->pages);

    if (pages > 0) code_op = resize_up(pages, pid_mem->pages);
    if (pages < 0) resize_down(pages * (-1), pid_mem->pages);

    retardo();
    send(client_fd, &code_op, sizeof(op_code), 0);
}

/*
    Itero desde la nueva posicion de pagina hasta que:
        -> Se recorrio todo el bitmap
        -> Se hayan asignado todas las paginas pedidas
    
    Si termino de iterar y pages es mayor a 0 significa que hubvieron paginas que no se pudieron asignar
*/
op_code resize_up(int pages, t_list* list_pages){
    sem_wait(&mutex_bit_map);
    for(int i = 0; i < bitarray_get_max_bit(bit_map) && pages > 0; i += 1){
        
        if(!bitarray_test_bit(bit_map, i)) { 
            uint32_t* frame = malloc(sizeof(uint32_t));
            *(frame) = i;
            list_add(list_pages, frame); 
            bitarray_set_bit(bit_map, i);
            pages -= 1;
        }
    }
    sem_post(&mutex_bit_map);

    if (pages > 0) return MEM_ERROR;

    return MEM_SUCCESS;
}

/* 
    Itero desde la ultima pagina hasta que:
        -> No haya mas paginas para liberar
        -> Ya libere todas las paginas
    
    Por cada iteracion remuevo la pagina y pongo en 0 el frame del bitmap correspondiente
*/
void resize_down(int pages, t_list* list_pages){
    for(int i = list_size(list_pages) - 1; i >= 0 && pages > 0; i -= 1){
        uint32_t* frame = list_remove(list_pages, i);

        sem_wait(&mutex_bit_map);
        bitarray_clean_bit(bit_map, *(frame));
        sem_post(&mutex_bit_map);

        free(frame);
        pages -= 1;
    }

}
