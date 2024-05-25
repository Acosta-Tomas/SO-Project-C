#include "main.h"

t_list* lista_memoria;
t_log* logger;

int main(int argc, char* argv[]) {
    t_config* config = config_create("memoria.config");

    if (config == NULL) exit(EXIT_FAILURE); 

    logger = log_create("memoria.logs", config_get_string_value(config, KEY_SERVER_LOG), true, LOG_LEVEL_DEBUG);

    int server_fd = iniciar_servidor(config_get_string_value(config, KEY_PUERTO_ESCUCHA));
    log_info(logger, "Server ready - SOCKET: %d", server_fd);

    lista_memoria = list_create();

    while (1) {
        int* cliente_fd = malloc(sizeof(int));
        *(cliente_fd) = esperar_cliente(server_fd);
        log_info(logger, "New client - SOCKET: %d", *(cliente_fd));

        pthread_t cliente;
        if (pthread_create(&cliente, NULL, memoria, (void*) cliente_fd)) {
            log_error(logger, "Problema al crear hilo cliente");
            exit(EXIT_FAILURE);
        } else pthread_detach(cliente);
    }

    close(server_fd);

    log_destroy(logger);
    config_destroy(config);
    
    return EXIT_SUCCESS;
}


void* memoria(void *client) {
    t_list* lista;
    int cliente_fd = *((int*) client);

    for (int cod_op = recibir_operacion(cliente_fd); cod_op != -1; cod_op = recibir_operacion(cliente_fd)){
        switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(cliente_fd, logger);
                break;
            case INIT_PID:
                t_init_pid* pid_to_init = recibir_init_process(cliente_fd, logger);

                op_code status = leer_archivo(pid_to_init->pid, pid_to_init->path);
                send(cliente_fd, &status, sizeof(op_code), 0);

                free(pid_to_init->path);
                free(pid_to_init);
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

    return EXIT_SUCCESS;
}


op_code leer_archivo(uint32_t pid, const char *nombre_archivo){
    FILE* archivo = fopen(nombre_archivo, "r");

    if(archivo == NULL){
        log_info(logger, "No se encontro el archivo deseasdo: %s", nombre_archivo); //Hacer log
        return INIT_PID_ERROR;
    }

    fseek(archivo, 0, SEEK_END);
    int tamaño_archivo = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    char* buffer = malloc(tamaño_archivo);

    t_memoria* pid_memoria = malloc(sizeof(t_memoria));
    pid_memoria->pid = pid;
    pid_memoria->file = list_create();

    while(fgets(buffer, tamaño_archivo, archivo) != NULL){
        log_info(logger, "instruccion %s", buffer);
        int size_line = strlen(buffer);
        if (!feof(archivo)) size_line -= 1; 
        char* file_line = malloc(size_line);
        memcpy(file_line, buffer, size_line);
        log_info(logger, "line %s", file_line);
        list_add(pid_memoria->file, file_line);
    }


    list_add(lista_memoria, pid_memoria);

    fclose(archivo);
    free(buffer);

    return INIT_PID_SUCCESS;
}

char** get_instruction_pid(uint32_t pid, uint32_t pc){
    bool* is_this_pid(void* element) {
        t_memoria* element_memoria = (t_memoria*) element;

        if (pid == element_memoria->pid) return true;

        return false;
    };

    t_memoria* memoria_pid = list_find(lista_memoria, is_this_pid);

    char* instruccion = (char*) list_get(memoria_pid->file, pc);

    char** instruccion_formateada = string_split(instruccion, " ");

    return instruccion_formateada;
}

void enviar_instruccion(int client_fd, char** array_instruction){
    t_paquete* paquete = crear_paquete(INSTRUCTION);

    for(int i = 0; i < string_array_size(array_instruction); i += 1){
        log_info(logger, "Instruccion: %s", array_instruction[i]);
        agregar_a_paquete(paquete, array_instruction[i], strlen(array_instruction[i]) + 1);
    }

    enviar_paquete(paquete, client_fd);
    eliminar_paquete(paquete);

}
                           