#include "main.h"

void* io_main(void *arg) {
    log_info(logger, "Thread entrada salida creado");


    int server_fd = iniciar_servidor(config_get_string_value(config, KEY_PUERTO_ESCUCHA));
    log_info(logger, "Server ready - SOCKET: %d", server_fd);

    while (1) {
        
        int* cliente_fd = malloc(sizeof(int));
        *(cliente_fd) = esperar_cliente(server_fd);

        log_info(logger, "New client - SOCKET: %d", *(cliente_fd));

        pthread_t cliente;
        
        if (pthread_create(&cliente, NULL, io_client, (void*) cliente_fd)) {
            log_error(logger, "Problema al crear hilo cliente");
            exit(EXIT_FAILURE);
        } else pthread_detach(cliente);
    }
        
    close(server_fd);
    return EXIT_SUCCESS;
}

void* io_client(void *client) {
    int cliente_fd = *((int*) client);

    recibir_operacion(cliente_fd);
    char* nombre_cliente = recibir_mensaje(cliente_fd);

    log_info(logger, "Interfaz: %s", nombre_cliente);

    t_io_client* io_client = malloc(sizeof(t_io_client));

    sem_init(&io_client->hay_io, 0, 0);
    sem_init(&io_client->mutex_io, 0, 1);
    
    io_client->queue_io = queue_create();

    sem_wait(&mutex_io_clients);
    dictionary_put(dict_io_clients, nombre_cliente, io_client);
    sem_post(&mutex_io_clients);

    bool bad_op = false;

    while (!bad_op){
        sem_wait(&io_client->hay_io);
        sem_wait(&io_client->mutex_io);
        t_io_queue* io = queue_pop(io_client->queue_io);
        sem_post(&io_client->mutex_io);

        t_paquete* paquete = crear_paquete(IO);
    
        agregar_io_serializado(paquete,  io->io_info);
        enviar_paquete(paquete, cliente_fd);
        eliminar_paquete(paquete);

        free(io->io_info->buffer);
        free(io->io_info);

        op_code cod_op = recibir_operacion(cliente_fd);

        switch (cod_op){
            case IO_ERROR:
                log_error(logger, "Error en IO %u", io->pcb->pid);
                free(io->pcb->registers);
                free(io->pcb);
                break;

            case IO_SUCCESS:
                log_info(logger, "Fin IO proceso a ready: %u", io->pcb->pid);

                sem_wait(&mutex_ready);
                queue_push(queue_ready, io->pcb);
                sem_post(&mutex_ready);
                sem_post(&hay_ready);
            break;
            
            default:
                bad_op = true;
                break;
        }

        free(io);
    }


    sem_wait(&mutex_io_clients);
    dictionary_remove(dict_io_clients, nombre_cliente);
    sem_post(&mutex_io_clients);

    queue_destroy(io_client->queue_io);
    sem_destroy(&io_client->hay_io);
    sem_destroy(&io_client->mutex_io);


    log_error(logger, "Client disconnected - SOCKET: %d", cliente_fd);
    free(client);
    
    return EXIT_SUCCESS;
}