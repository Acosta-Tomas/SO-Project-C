#include "main.h"

void* io_main(void *arg) {
    log_info(logger, "Thread entrada salida creado");


    int server_fd = iniciar_servidor(config_get_string_value(config, KEY_PUERTO_ESCUCHA));
    log_info(logger, "Server ready - SOCKET: %d", server_fd);

    while(1) {
        int cliente_fd = esperar_cliente(server_fd);
        log_info(logger, "New client - SOCKET: %d", cliente_fd);

        bool bad_op = false;

        while (!bad_op){
            sem_wait(&hay_io);
            sem_wait(&mutex_io);
            t_io* io = queue_pop(queue_io);
            sem_post(&mutex_io);

            t_paquete* paquete = crear_paquete(IO);
        
            agregar_io_paquete(paquete, io->type_instruction, io->name_interface, io->sleep_time);
            enviar_paquete(paquete, cliente_fd);
            eliminar_paquete(paquete);
            free(io->name_interface);
            free(io->sleep_time);
            free(io);

            op_code cod_op = recibir_operacion(cliente_fd);
        
            sem_wait(&mutex_blocked);
            t_pcb* pcb = queue_pop(queue_blocked);
            sem_post(&mutex_blocked);

            switch (cod_op){
                case IO_ERROR:
                    log_error(logger, "Error en IO %u", pcb->pid);
                    free(pcb->registers);
                    free(pcb);
                    break;

                case IO_SUCCESS:
                    log_info(logger, "Fin IO proceso a ready: %u", pcb->pid);

                    sem_wait(&mutex_ready);
                    queue_push(queue_ready, pcb);
                    sem_post(&mutex_ready);
                    sem_post(&hay_ready);
                break;
                
                default:
                    bad_op = true;
                    break;
            }
        }

        log_error(logger, "Client disconnected - SOCKET: %d", cliente_fd);
    }
        
    close(server_fd);
    return EXIT_SUCCESS;
}