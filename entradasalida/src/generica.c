#include "main.h"

void generica_io(char* nombre, t_config* config){
    t_log* logger = log_create("GENERICA.logs", config_get_string_value(config, KEY_LOGGER), true, LOG_LEVEL_INFO);

    char* ip_kernel = config_get_string_value(config, KEY_IP_KERNEL);
    char* puerto_kernel = config_get_string_value(config, KEY_PUERTO_KERNEL);
    char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);
    int unidad_trabajo = config_get_int_value(config, KEY_UNIDAD_TRABAJO);

    int kernel_fd = crear_conexion(ip_kernel, puerto_kernel);
    log_info(logger, "Connected to Kernel -  SOCKET: %d", kernel_fd);

    int memoria_fd = crear_conexion(ip_memoria, puerto_memoria);
    log_info(logger, "Connected to Memoria -  SOCKET: %d", memoria_fd);

    enviar_mensaje(nombre, kernel_fd, MENSAJE);

    for (op_code cod_op = recibir_operacion(kernel_fd); cod_op != -1; cod_op = recibir_operacion(kernel_fd)){
        if (cod_op == IO){
            t_io* io = recibir_io_serializado(kernel_fd, logger);

            if(io->type_instruction != IO_GEN_SLEEP) {
                op_code to_send = IO_ERROR;
                send(kernel_fd, &to_send, sizeof(op_code), 0);
                log_info(logger, "Instruccion no valida, se avisa a kernel");

                free(io->buffer);
                free(io);
                continue;
            }

            char* sleep_time = malloc(io->buffer_size);
            memcpy(sleep_time, io->buffer, io->buffer_size);

            log_info(logger, "Se solicita: %s - %s segundos", nombre, sleep_time);
            usleep(atoi(sleep_time) * unidad_trabajo * 1000);

            op_code to_send = IO_SUCCESS;
            send(kernel_fd, &to_send, sizeof(op_code), 0);
            log_info(logger, "Finalizada instruccion, se avisa a kernel");

            free(sleep_time);
            free(io->buffer);
            free(io);
        }
    }

   
	log_destroy(logger);
	liberar_conexion(kernel_fd);
    liberar_conexion(memoria_fd);
}