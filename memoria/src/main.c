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

                           