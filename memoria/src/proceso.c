#include "main.h"

op_code leer_script(char* nombre_script, char** comandos){
    FILE* archivo = open_file(nombre_script);

    if(archivo == NULL) return INIT_SCRIPT_ERROR;

    int tamaño_archivo = get_file_size(archivo);

    char* buffer = malloc(tamaño_archivo);

    while(fgets(buffer, tamaño_archivo, archivo) != NULL){
        string_append(comandos, buffer);
    }

    return INIT_SCRIPT_SUCCESS;
}

op_code leer_archivo(uint32_t pid, char *nombre_archivo){
    FILE* archivo = open_file(nombre_archivo);

    if(archivo == NULL) return INIT_PID_ERROR;

    int tamaño_archivo = get_file_size(archivo);

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

    return INIT_PID_SUCCESS;
}

FILE* open_file(char* nombre){
    char* full_path = string_new();
    string_append(&full_path, path_scripts);
    string_append(&full_path, nombre);
    FILE* archivo = fopen(full_path, "r");

    log_debug(logger, "Archivo a leer: %s", full_path);

    free(full_path);

    return archivo;
}

int get_file_size(FILE* archivo){
    fseek(archivo, 0, SEEK_END);
    int tamaño_archivo = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    return tamaño_archivo;
}

void get_instruction(int client_fd){
    uint32_t pid, pc;

    pid = recibir_pid_con_uint32(client_fd, &pc, logger);
    enviar_instruccion(client_fd, get_instruction_pid(pid, pc));     

    log_debug(logger, "PID: %u - PC: %u enviado a CPU", pid, pc);  
}

char** get_instruction_pid(uint32_t pid, uint32_t pc){
    t_memoria* memoria_pid;

    memoria_pid = get_pid_mem(pid);

    char* instruccion = *(memoria_pid->file + pc);
    char** instruccion_formateada = string_split(instruccion, " ");

    return instruccion_formateada;
}

void enviar_instruccion(int client_fd, char** array_instruction){
    t_paquete* paquete = crear_paquete(INSTRUCTION);

    for(int i = 0; i < string_array_size(array_instruction); i += 1){
        agregar_a_paquete(paquete, array_instruction[i], strlen(array_instruction[i]) + 1);
        free(array_instruction[i]);
    }
    
    free(array_instruction);

    retardo();
    enviar_paquete(paquete, client_fd);
    eliminar_paquete(paquete);
}

 t_memoria* get_pid_mem(uint32_t pid){
    t_memoria* pid_mem;

    sem_wait(&mutex_mem_procesos);
    pid_mem = dictionary_get(memoria_procesos, string_itoa((int) pid));
    sem_post(&mutex_mem_procesos);

    return pid_mem;
}

void get_pid_page(int client_fd) {
    uint32_t pid, page;
    t_memoria* pid_mem;

    pid = recibir_pid_con_uint32(client_fd, &page, logger);
    pid_mem = get_pid_mem(pid);

    if (page >= list_size(pid_mem->pages)) {
        op_code code = MEM_ERROR;
        send(client_fd, &code, sizeof(op_code), 0);
        return;
    }

    uint32_t* frame = list_get(pid_mem->pages, page);

    log_info(logger, "PID: %u - Pagina: %u - Marco: %u", pid, page, *frame);

    retardo();
    t_paquete* paquete = crear_paquete(MEM_SUCCESS);

    agregar_uint_a_paquete(paquete, frame, sizeof(uint32_t));
    enviar_paquete(paquete, client_fd);
    eliminar_paquete(paquete);
}