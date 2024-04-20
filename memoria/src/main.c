#include "main.h"

int main(int argc, char* argv[]) {
    t_config* config = config_create("memoria.config");

    if (config == NULL) exit(EXIT_FAILURE); 

    t_log* logger = log_create("memoria.logs", config_get_string_value(config, KEY_SERVER_LOG), true, LOG_LEVEL_DEBUG);

    void myiterator(char* value) {
        log_info(logger,"%s", value);
    }

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
                case PAQUETE:
                    lista = recibir_paquete(cliente_fd);
                    log_info(logger, "Me llegaron los siguientes valores:\n");
                    list_iterate(lista, (void*) myiterator); // Ver alguna manera de pasar un log al iterator, por ahi ahora es relevanto pero desp no
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
