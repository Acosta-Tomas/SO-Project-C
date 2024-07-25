#include "main.h"

void* memoria(void *client) {
    int cliente_fd = *((int*) client);

    for (int cod_op = recibir_operacion(cliente_fd); cod_op != -1; cod_op = recibir_operacion(cliente_fd)){
        switch (cod_op) {
            case MENSAJE: recibir_mensaje(cliente_fd); break;

            case MEM_PAGE_SIZE: send(cliente_fd, &page_size, sizeof(page_size), 0); break;

            case MEM_RESIZE: resize_process(cliente_fd); break;
            
            case MEM_PID_PAGE: get_pid_page(cliente_fd); break;

            case MEM_WRITE: escribir_memoria(cliente_fd); break;

            case MEM_READ: leer_memoria(cliente_fd); break;

            case INIT_PID: init_proceso(cliente_fd); break;
            
            case INIT_SCRIPT: init_script(cliente_fd); break;

            case END_PID: liberar_memoria(cliente_fd); break;
                
            case INSTRUCTION: get_instruction(cliente_fd); break;

            default: log_warning(logger,"Operacion desconocida. No quieras meter la pata"); break;
        }
    }

    log_error(logger, "SOCKET: %d - Cliente desconectado", cliente_fd);
    free(client);

    return EXIT_SUCCESS;
}

void liberar_memoria(int client_fd){
    uint32_t pid, size_discard;
    t_memoria* pid_mem;

    recv(client_fd, &size_discard, sizeof(u_int32_t), MSG_WAITALL); // Recibo tamaño buffer pero no me interesa (se manda por la serializacion de codigo de operacion con uint32)
    recv(client_fd, &pid, sizeof(uint32_t), MSG_WAITALL);

    sem_wait(&mutex_mem_procesos);
    pid_mem = dictionary_remove(memoria_procesos, string_itoa((int) pid));
    sem_post(&mutex_mem_procesos);

    resize_down(list_size(pid_mem->pages), pid_mem->pages);

    list_destroy(pid_mem->pages);
    string_array_destroy(pid_mem->file);
    free(pid_mem);

    log_info(logger, "PID: %u - Liberada memoria", pid);
}

void init_proceso(int client_fd){
    t_init_pid* pid_to_init = recibir_init_process(client_fd, logger);

    op_code status = leer_archivo(pid_to_init->pid, pid_to_init->path);
    retardo();
    send(client_fd, &status, sizeof(op_code), 0);

    free(pid_to_init->path);
    free(pid_to_init);
}

void init_script(int client_fd){
    char* archivo = recibir_mensaje(client_fd);
    char* comandos = string_new();

    op_code codigo = leer_script(archivo, &comandos);

    retardo();
    
    if (codigo == INIT_SCRIPT_ERROR) send(client_fd, &codigo, sizeof(op_code), 0);
    else enviar_mensaje(comandos, client_fd, codigo);

    free(comandos);
}

