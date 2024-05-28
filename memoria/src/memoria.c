#include "main.h"

void* memoria(void *client) {
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
        log_info(logger, "No se encontro el archivo deseasdo: %s", nombre_archivo);
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
        int size_line = strlen(buffer);
        if (!feof(archivo)) size_line -= 1; 
        char* file_line = malloc(size_line);
        memcpy(file_line, buffer, size_line);
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
        agregar_a_paquete(paquete, array_instruction[i], strlen(array_instruction[i]) + 1);
    }

    enviar_paquete(paquete, client_fd);
    eliminar_paquete(paquete);

}