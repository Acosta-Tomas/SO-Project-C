#include "main.h"


int main(int argc, char* argv[]) {
    t_config* config = config_create("memoria.config");

    if (config == NULL) exit(EXIT_FAILURE); 

    t_log* logger = log_create("memoria.logs", config_get_string_value(config, KEY_SERVER_LOG), true, LOG_LEVEL_DEBUG);

    int server_fd = iniciar_servidor(config_get_string_value(config, KEY_PUERTO_ESCUCHA));

    log_info(logger, "Server ready - SOCKET: %d", server_fd);

    while (1) {
        int cliente_fd = esperar_cliente(server_fd);
        log_info(logger, "New client - SOCKET: %d", cliente_fd);

        t_list* lista;

        for (int cod_op = recibir_operacion(cliente_fd); cod_op != -1; cod_op = recibir_operacion(cliente_fd)){
            switch (cod_op) {
                case MENSAJE:
                    recibir_mensaje(cliente_fd, logger);
                    break;
                case INIT_PID:
                    lista = recibir_paquete(cliente_fd);
                    log_info(logger, "Me llegaron los siguientes valores:\n");
                    void myiterator(char* value) {
                        log_info(logger,"%s", value);
                    }
                    list_iterate(lista, (void*) myiterator);
                    op_code success = INIT_PID_SUCCESS;
                    send(cliente_fd, &success, sizeof(op_code), 0);
                    break;
                 
                case INSTRUCTION:
                    uint32_t pc;
                    uint32_t pid;
                    int tmñ;
                    recv(cliente_fd, &tmñ, sizeof(int), MSG_WAITALL);
                    recv(cliente_fd, &pc, sizeof(uint32_t), MSG_WAITALL);
                    recv(cliente_fd, &pid, sizeof(uint32_t), MSG_WAITALL); 

                    log_info(logger, "PC %u", pc);
                    log_info(logger, "PID %u", pid);

                    if (pc == 0) {
                        char* mensaje1 = "SET";
                        char* mensaje2 = "AX";
                        char* mensaje3 = "20";
                        t_paquete* pc_paquete = crear_paquete(INSTRUCTION);
                        agregar_a_paquete(pc_paquete, mensaje1, strlen(mensaje1) + 1);
                        agregar_a_paquete(pc_paquete, mensaje2, strlen(mensaje2) + 1);
                        agregar_a_paquete(pc_paquete, mensaje3, strlen(mensaje3) + 1);
                        enviar_paquete(pc_paquete, cliente_fd);
                    }

                    if (pc == 1) {
                        char* mensaje1 = "SET";
                        char* mensaje2 = "BX";
                        char* mensaje3 = "10";
                        t_paquete* pc_paquete = crear_paquete(INSTRUCTION);
                        agregar_a_paquete(pc_paquete, mensaje1, strlen(mensaje1) + 1);
                        agregar_a_paquete(pc_paquete, mensaje2, strlen(mensaje2) + 1);
                        agregar_a_paquete(pc_paquete, mensaje3, strlen(mensaje3) + 1);
                        enviar_paquete(pc_paquete, cliente_fd);
                    }

                    if (pc == 2) {
                        char* mensaje1 = "SUB";
                        char* mensaje2 = "AX";
                        char* mensaje3 = "BX";
                        t_paquete* pc_paquete = crear_paquete(INSTRUCTION);
                        agregar_a_paquete(pc_paquete, mensaje1, strlen(mensaje1) + 1);
                        agregar_a_paquete(pc_paquete, mensaje2, strlen(mensaje2) + 1);
                        agregar_a_paquete(pc_paquete, mensaje3, strlen(mensaje3) + 1);
                        enviar_paquete(pc_paquete, cliente_fd);
                    }

                    if (pc == 3) {
                        char* mensaje1 = "JNZ";
                        char* mensaje2 = "AX";
                        char* mensaje3 = "2";
                        t_paquete* pc_paquete = crear_paquete(INSTRUCTION);
                        agregar_a_paquete(pc_paquete, mensaje1, strlen(mensaje1) + 1);
                        agregar_a_paquete(pc_paquete, mensaje2, strlen(mensaje2) + 1);
                        agregar_a_paquete(pc_paquete, mensaje3, strlen(mensaje3) + 1);
                        enviar_paquete(pc_paquete, cliente_fd);
                    }           

                    if (pc == 4) {
                        char* mensaje1 = "EXIT";
                        t_paquete* pc_paquete = crear_paquete(INSTRUCTION);
                        agregar_a_paquete(pc_paquete, mensaje1, strlen(mensaje1) + 1);
                        enviar_paquete(pc_paquete, cliente_fd);
                    }                                     
   
                    break;
                default:
                    log_warning(logger,"Operacion desconocida. No quieras meter la pata");
                    break;
            }
        }

        log_error(logger, "Client disconnected - SOCKET: %d", cliente_fd);
    }


    close(server_fd);

    log_destroy(logger);
    config_destroy(config);
    
    return EXIT_SUCCESS;
}

t_list* lista_memoria;

void leer_archivo(const char *nombre_archivo){

    FILE* archivo = fopen(nombre_archivo, "r");

    if(archivo == NULL){
        printf("NO EXISTE EL ARCHIVO\n"); //Hacer log
        return;
    }

    lista_memoria = list_create(); //Inicializar en main

    fseek(archivo, 0, SEEK_END);
    long tamaño_archivo = ftell(archivo);

    void * buffer = malloc(tamaño_archivo);

    t_list* lista_pid = list_create();

    while(fgets(buffer, sizeof(buffer), archivo) != NULL){
        list_add(lista_pid, buffer);
    }

    list_add(lista_memoria, lista_pid);

    fclose(archivo);

    //Falta liberar
}

void mandar_a_cpu(t_list* lista_memoria, int PID, int PC){
    
    t_list *lista_memoria_accedida = list_get(lista_memoria, PID);

    void * instruccion = list_get(lista_memoria_accedida, PC);

    char *instruccion_formateada = string_split(instruccion, " ");

    //t_log* log_instruccion = log_create("instruccion.log", "Logg de instruccion", true, LOG_LEVEL_DEBUG);
    //log_info(log_instruccion, instruccion_formateada);

    //Mandar a CPU

}
