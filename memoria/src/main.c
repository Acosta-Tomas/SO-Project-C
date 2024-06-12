#include "main.h"

t_dictionary* memoria_procesos;
void* memoria_usuario;
t_bitarray* bit_map;

uint32_t page_size;
uint32_t max_pages;

t_log* logger;
t_config* config;

sem_t mutex_bit_map;
sem_t mutex_mem_usuario;
sem_t mutex_mem_procesos;


int main(int argc, char* argv[]) {
    config = config_create("memoria.config");

    if (config == NULL) exit(EXIT_FAILURE); 

    logger = log_create("memoria.logs", config_get_string_value(config, KEY_SERVER_LOG), true, LOG_LEVEL_DEBUG);

    int server_fd = iniciar_servidor(config_get_string_value(config, KEY_PUERTO_ESCUCHA));
    log_info(logger, "Server ready - SOCKET: %d", server_fd);

    memoria_procesos = dictionary_create();

    uint32_t mem_size = (uint32_t) config_get_int_value(config, KEY_TAM_MEMORIA);
    page_size = (uint32_t) config_get_int_value(config, KEY_TAM_PAGINA);
    max_pages = mem_size/page_size;
    memoria_usuario = calloc(1, mem_size);

    void* bits = calloc(1, max_pages/8);

    if (bits == NULL) {
        perror("Error al asignar memoria");
        return EXIT_FAILURE;
    }

    bit_map = bitarray_create_with_mode(bits, max_pages/8, MSB_FIRST);

    sem_init(&mutex_bit_map, 0, 1);
    sem_init(&mutex_mem_usuario, 0, 1);
    sem_init(&mutex_mem_procesos, 0, 1);

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

    sem_destroy(&mutex_bit_map);
    sem_destroy(&mutex_mem_procesos);
    sem_destroy(&mutex_mem_usuario);

    log_destroy(logger);
    config_destroy(config);
    bitarray_destroy(bit_map);
    free(bits);
    free(memoria_usuario);
    
    return EXIT_SUCCESS;
}

                           