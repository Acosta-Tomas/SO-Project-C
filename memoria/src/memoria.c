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
    char* instructions = string_new();

    while(fgets(buffer, tamaño_archivo, archivo) != NULL){
        string_append(&instructions, buffer);
    }

    log_info(logger, "Archivo Leido: %s tam: %ld", instructions, strlen(instructions));


    t_memoria* pid_memoria = malloc(sizeof(t_memoria));
    pid_memoria->pid = pid;
    pid_memoria->file = string_split(instructions, "\n");
    pid_memoria->pages = list_create();

    log_info(logger, "Cantidad instrucciones: %i", string_array_size(pid_memoria->file));

    dictionary_put(memoria_procesos, string_itoa((int) pid), pid_memoria);

    fclose(archivo);
    free(buffer);
    free(instructions);

    return INIT_PID_SUCCESS;
}

char** get_instruction_pid(uint32_t pid, uint32_t pc){
    t_memoria* memoria_pid = dictionary_get(memoria_procesos, string_itoa((int) pid));

    char* instruccion = *(memoria_pid->file + pc);

    char** instruccion_formateada = string_split(instruccion, " ");

    return instruccion_formateada;
}

void enviar_instruccion(int client_fd, char** array_instruction){
    t_paquete* paquete = crear_paquete(INSTRUCTION);

    for(int i = 0; i < string_array_size(array_instruction); i += 1){
        agregar_a_paquete(paquete, array_instruction[i], strlen(array_instruction[i]) + 1);
    }

    usleep(config_get_int_value(config, KEY_RETARDO_RESPUESTA) * 1000);

    enviar_paquete(paquete, client_fd);
    eliminar_paquete(paquete);
}