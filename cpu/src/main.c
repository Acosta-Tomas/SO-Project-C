#include "main.h"

t_log* logger;
t_config* config;
t_pcb* pcb;

sem_t run;
sem_t wait;
sem_t mutex_registros;

int main(int argc, char* argv[]) {
    config = config_create(CONFIG_FILE);

    if (config == NULL) exit(EXIT_FAILURE); 

    logger = log_create(LOGS_FILE, config_get_string_value(config, KEY_SERVER_LOG), true, LOG_LEVEL_DEBUG);

    
    int dispatch_fd = iniciar_servidor(config_get_string_value(config, KEY_PUERTO_ESCUCHA_DISPATCH));
    log_info(logger, "Server ready - SOCKET: %d", dispatch_fd);

    char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);
    int memoria_fd = crear_conexion(ip_memoria, puerto_memoria);

    log_info(logger, "Connected to Memoria - SOCKET: %d", memoria_fd);

    while(1) {
        int kernel_fd = esperar_cliente(dispatch_fd);
        log_info(logger, "New client - SOCKET: %d", kernel_fd);

        for (int cod_op = recibir_operacion(kernel_fd); cod_op != -1; cod_op = recibir_operacion(kernel_fd)){
            if (cod_op == PCB) {
                pcb = recibir_pcb(memoria_fd, logger);

                cpu(memoria_fd, kernel_fd);
            }
        }
    }
     liberar_conexion(memoria_fd);
    close(dispatch_fd);
    log_destroy(logger);
    config_destroy(config);

    return EXIT_SUCCESS;
}
