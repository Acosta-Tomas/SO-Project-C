#include "main.h"

void stdout_io(char* nombre, t_config* config){
    t_log* logger = log_create("STDOUT.logs", config_get_string_value(config, KEY_LOGGER), true, log_level_from_string(config_get_string_value(config, KEY_LOG_LEVEL)));

    char* ip_kernel = config_get_string_value(config, KEY_IP_KERNEL);
    char* puerto_kernel = config_get_string_value(config, KEY_PUERTO_KERNEL);
    char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);
    // int unidad_trabajo = config_get_int_value(config, KEY_UNIDAD_TRABAJO);

    int kernel_fd = crear_conexion(ip_kernel, puerto_kernel);
    log_info(logger, "SOCKET: %d - Kernel", kernel_fd);

    int memoria_fd = crear_conexion(ip_memoria, puerto_memoria);
    log_info(logger, "SOCKET: %d - Memoria", memoria_fd);

    enviar_mensaje(nombre, kernel_fd, MENSAJE);

    for (op_code cod_op = recibir_operacion(kernel_fd); cod_op != -1; cod_op = recibir_operacion(kernel_fd)){
        if (cod_op == IO){
            uint32_t pid;
            t_io* io = recibir_io_serializado(kernel_fd, &pid, logger);

            if(io->type_instruction != IO_STDOUT_WRITE){
                op_code to_send = IO_ERROR;
                send(kernel_fd, &to_send, sizeof(op_code), 0);
                log_error(logger, "Instruccion no valida, se avisa a kernel");

                free(io->buffer);
                free(io);
                continue;
            }

            log_info(logger, "PID: %u - Operacion: IO_STDOUT_WRITE", pid);

            t_list* frames = list_create();
            uint32_t tamaño_a_leer = get_io_frames(io, frames);
            char* buffer = malloc(tamaño_a_leer);
            int status = leer_memoria(memoria_fd, buffer, frames, logger, pid);
            
            op_code to_send = status == -1 ? IO_ERROR : IO_SUCCESS;

            printf("Datos leidos: %.*s\n", tamaño_a_leer, buffer);

            send(kernel_fd, &to_send, sizeof(op_code), 0);

            free(io->buffer);
            free(io);
            free(buffer);
            list_destroy(frames);
        }
    }

	log_destroy(logger);
	liberar_conexion(kernel_fd);
    liberar_conexion(memoria_fd);
}

