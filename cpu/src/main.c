#include "main.h"

t_log* logger;
t_registros* registros;
t_config* config;

sem_t run;
sem_t wait;
sem_t mutex_registros;

int main(int argc, char* argv[]) {
    config = config_create(CONFIG_FILE);

    if (config == NULL) exit(EXIT_FAILURE); 

    sem_init(&run, 0, 0);
    sem_init(&wait, 0, 1);
    sem_init(&mutex_registros, 0, 1);

    logger = log_create(LOGS_FILE, config_get_string_value(config, KEY_SERVER_LOG), true, LOG_LEVEL_DEBUG);

    char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);
    int conexion = crear_conexion(ip_memoria, puerto_memoria);

    log_info(logger, "Connected to Memoria - SOCKET: %d", conexion);
    
    registros = create_registros();

    sem_post(&run);
    
    cpu(conexion);

    liberar_conexion(conexion);


    // void myiterator(char* value) {
    //     log_info(logger,"%s", value);
    // }

    // int server_fd = iniciar_servidor(config_get_string_value(config, KEY_PUERTO_ESCUCHA_DISPATCH));
    // log_info(logger, "Server ready - SOCKET: %d", server_fd);

    // while(1) {
    //     int cliente_fd = esperar_cliente(server_fd);
    //     log_info(logger, "New client - SOCKET: %d", cliente_fd);

    //     t_list* lista;

    //     for (int cod_op = recibir_operacion(cliente_fd); cod_op != -1; cod_op = recibir_operacion(cliente_fd)){
    //         switch (cod_op) {
    //             case MENSAJE:
    //                 recibir_mensaje(cliente_fd, logger);
    //                 break;
    //             case PAQUETE:
    //                 lista = recibir_paquete(cliente_fd);
    //                 log_info(logger, "Me llegaron los siguientes valores:\n");
    //                 list_iterate(lista, (void*) myiterator); // Ver alguna manera de pasar un log al iterator, por ahi ahora es relevanto pero desp no
    //                 break;
    //             default:
    //                 log_warning(logger,"Operacion desconocida. No quieras meter la pata");
    //                 break;
    //         }
    //     }

    //     log_error(logger, "Client disconnected - SOCKET: %d", cliente_fd);

    //     char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    //     char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);
    //     int conexion = crear_conexion(ip_memoria, puerto_memoria);

    //     log_info(logger, "Connected to Memoria - SOCKET: %d", conexion);

    //     t_paquete * paquete = crear_paquete();

    //     void mi_paquete_add(char * value) {
    //         agregar_a_paquete(paquete, value, strlen(value) + 1);
    //     }

    //     list_iterate(lista, (void *) mi_paquete_add);

    //     enviar_paquete(paquete, conexion);
    
    //     eliminar_paquete(paquete);
    //     liberar_conexion(conexion);
    // }

    sem_destroy(&run);
    sem_destroy(&wait);
    free(registros);
    // close(server_fd);
    log_destroy(logger);
    config_destroy(config);

    return EXIT_SUCCESS;
}
