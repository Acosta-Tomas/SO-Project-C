#include "main.h"

void* memoria(void *client) {
    int cliente_fd = *((int*) client);

    for (int cod_op = recibir_operacion(cliente_fd); cod_op != -1; cod_op = recibir_operacion(cliente_fd)){
        switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(cliente_fd);
                break;

            case MEM_PAGE_SIZE:
                send(cliente_fd, &page_size, sizeof(page_size), 0);
                break;

            case MEM_RESIZE:
                resize_process(cliente_fd);
                break;
            
            case MEM_PID_PAGE:
                get_pid_page(cliente_fd);
                break;

            case MEM_WRITE: 
                escribir_memoria(cliente_fd);
                break;

            case MEM_READ: 
                leer_memoria(cliente_fd);
                break;

            case INIT_PID:
                t_init_pid* pid_to_init = recibir_init_process(cliente_fd, logger);

                op_code status = leer_archivo(pid_to_init->pid, pid_to_init->path);
                send(cliente_fd, &status, sizeof(op_code), 0);

                free(pid_to_init->path);
                free(pid_to_init);
                break;
            
            case INIT_SCRIPT:
                char* archivo = recibir_mensaje(cliente_fd);
                char* comandos = string_new();

                op_code codigo = leer_script(archivo, &comandos);
                
                if (codigo == INIT_SCRIPT_ERROR) send(cliente_fd, &codigo, sizeof(op_code), 0);
                else enviar_mensaje(comandos, cliente_fd, codigo);

                break;

            case END_PID:
                liberar_memoria(cliente_fd);
                break;
                
            case INSTRUCTION:
                uint32_t pc;
                uint32_t pid;
                int tmñ;

                recv(cliente_fd, &tmñ, sizeof(int), MSG_WAITALL);
                recv(cliente_fd, &pc, sizeof(uint32_t), MSG_WAITALL);
                recv(cliente_fd, &pid, sizeof(uint32_t), MSG_WAITALL);

                char** array_instruction = get_instruction_pid(pid, pc);
                enviar_instruccion(cliente_fd, array_instruction);     

                log_info(logger, "PID: %u - PC: %u enviado a CPU", pid, pc);  
                break;
            default:
                log_warning(logger,"Operacion desconocida. No quieras meter la pata");
                break;
        }
    }

    log_error(logger, "Client disconnected - SOCKET: %d", cliente_fd);
    free(client);

    return EXIT_SUCCESS;
}

op_code leer_script(char* nombre_script, char** comandos){
    char* path = config_get_string_value(config, KEY_PATH_INSTRUCCIONES);
    char* full_path = string_new();
    string_append(&full_path, path);
    string_append(&full_path, nombre_script);
    FILE* archivo = fopen(full_path, "r");

    log_info(logger, "Script a leer: %s", full_path);

    if(archivo == NULL){
        log_info(logger, "No se encontro el script deseasdo: %s", path);
        return INIT_SCRIPT_ERROR;
    }

    fseek(archivo, 0, SEEK_END);
    int tamaño_archivo = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    char* buffer = malloc(tamaño_archivo);

    while(fgets(buffer, tamaño_archivo, archivo) != NULL){
        string_append(comandos, buffer);
    }

    return INIT_SCRIPT_SUCCESS;
}


op_code leer_archivo(uint32_t pid, char *nombre_archivo){
    char* path = config_get_string_value(config, KEY_PATH_INSTRUCCIONES);
    char* full_path = string_new();
    string_append(&full_path, path);
    string_append(&full_path, nombre_archivo);
    FILE* archivo = fopen(full_path, "r");

    log_info(logger, "Archivo a leer: %s", full_path);

    if(archivo == NULL){
        log_info(logger, "No se encontro el archivo deseasdo: %s", path);
        return INIT_PID_ERROR;
    }

    fseek(archivo, 0, SEEK_END);
    int tamaño_archivo = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    char* buffer = malloc(tamaño_archivo);
    char* instructions = string_new();

    while(fgets(buffer, tamaño_archivo, archivo) != NULL){
        string_append(&instructions, buffer);
    }


    t_memoria* pid_memoria = malloc(sizeof(t_memoria));
    pid_memoria->pid = pid;
    pid_memoria->file = string_split(instructions, "\n");
    pid_memoria->pages = list_create();

    sem_wait(&mutex_mem_procesos);
    dictionary_put(memoria_procesos, string_itoa((int) pid), pid_memoria);
    sem_post(&mutex_mem_procesos);

    fclose(archivo);
    free(buffer);
    free(instructions);
    free(full_path);

    return INIT_PID_SUCCESS;
}

char** get_instruction_pid(uint32_t pid, uint32_t pc){
    t_memoria* memoria_pid;

    memoria_pid = get_pid(pid);

    char* instruccion = *(memoria_pid->file + pc);
    char** instruccion_formateada = string_split(instruccion, " ");

    return instruccion_formateada;
}

void enviar_instruccion(int client_fd, char** array_instruction){
    t_paquete* paquete = crear_paquete(INSTRUCTION);

    for(int i = 0; i < string_array_size(array_instruction); i += 1){
        agregar_a_paquete(paquete, array_instruction[i], strlen(array_instruction[i]) + 1);
    }

    retardo();
    enviar_paquete(paquete, client_fd);
    eliminar_paquete(paquete);
}

void resize_process(int client_fd){
    uint32_t pid, size;
    t_memoria* pid_mem;
    op_code code_op = MEM_SUCCESS;

    recv(client_fd, &pid, sizeof(uint32_t), MSG_WAITALL);
    recv(client_fd, &size, sizeof(uint32_t), MSG_WAITALL);

    pid_mem = get_pid(pid);
    int pages = ceil((double)size / page_size) - list_size(pid_mem->pages);

    if (pages > 0) code_op = resize_up(pages, pid_mem->pages);
    if (pages < 0) resize_down(pages * (-1), pid_mem->pages);

    retardo();

    send(client_fd, &code_op, sizeof(op_code), 0);
}

void get_pid_page(int client_fd) {
    uint32_t pid, page;
    t_memoria* pid_mem;
    uint32_t* frame;

    recv(client_fd, &pid, sizeof(uint32_t), MSG_WAITALL);
    recv(client_fd, &page, sizeof(uint32_t), MSG_WAITALL);

    pid_mem = get_pid(pid);

    int s = list_size(pid_mem->pages);

    if (page >= s) {
        op_code code = MEM_ERROR;
        send(client_fd, &code, sizeof(op_code), 0);
        return;
    }

    frame = list_get(pid_mem->pages, page);

    retardo();

    op_code code = MEM_SUCCESS;
    send(client_fd, &code, sizeof(op_code), 0);
    send(client_fd, frame, sizeof(uint32_t), 0);

}

void escribir_memoria(int client_fd){
    int size;
    int desplazamiento = 0;
    void* buffer;
    uint32_t direccion_fisica;
	int value_size;

    buffer = recibir_buffer(&size, client_fd);

    memcpy(&direccion_fisica, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&value_size, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
	void* value = malloc(value_size);
    memcpy(value, buffer + desplazamiento, value_size);
    desplazamiento += value_size;

    if (size != desplazamiento) log_info(logger, "Error al recibir PID Para iniciar");

	free(buffer);

    void* memoria_desplazada = memoria_usuario + direccion_fisica;

    sem_wait(&mutex_mem_usuario);
    memcpy(memoria_desplazada, value, value_size);
    sem_post(&mutex_mem_usuario);

    retardo();

    op_code code_op = MEM_SUCCESS;

    send(client_fd, &code_op, sizeof(op_code), 0);
}

void leer_memoria(int client_fd){
    uint32_t direccion_fisica;
    int buffer_size;

    recv(client_fd, &direccion_fisica, sizeof(uint32_t), MSG_WAITALL);
    recv(client_fd, &buffer_size, sizeof(uint32_t), MSG_WAITALL);

    void* buffer = malloc(buffer_size);

    void* memoria_desplazada = memoria_usuario + direccion_fisica;

    sem_wait(&mutex_mem_usuario);
    memcpy(buffer, memoria_desplazada, buffer_size);
    sem_post(&mutex_mem_usuario);
    retardo();

    op_code code_op = MEM_SUCCESS;
    send(client_fd, &code_op, sizeof(op_code), 0);
    send(client_fd, buffer, buffer_size, 0);
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
        
        if(!bitarray_test_bit(bit_map, i)) { // hace falta mutex para leer el bit?
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

void liberar_memoria(int client_fd){
    uint32_t pid;
    t_memoria* pid_mem;

    recv(client_fd, &pid, sizeof(uint32_t), MSG_WAITALL);

    sem_wait(&mutex_mem_procesos);
    pid_mem = dictionary_remove(memoria_procesos, string_itoa((int) pid));
    sem_post(&mutex_mem_procesos);

    resize_down(list_size(pid_mem->pages), pid_mem->pages); // Libera el bit array simulando que el resize del proceso es 0;
    mem_hexdump(bit_map->bitarray, bit_map->size);
    list_destroy(pid_mem->pages);
    string_array_destroy(pid_mem->file);
    free(pid_mem);

    log_info(logger, "PID: %u - Liberada memoria", pid);
}

 t_memoria* get_pid(uint32_t pid){
     t_memoria* pid_mem;
    sem_wait(&mutex_mem_procesos);
    pid_mem = dictionary_get(memoria_procesos, string_itoa((int) pid));
    sem_post(&mutex_mem_procesos);

    return pid_mem;
}

void retardo(void){
    usleep(config_get_int_value(config, KEY_RETARDO_RESPUESTA) * 1000);
}