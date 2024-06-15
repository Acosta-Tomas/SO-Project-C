#include "main.h"

int main(int argc, char* argv[]) {
    t_config* config = config_create(CONFIG_FILE);

    if (config == NULL) exit(EXIT_FAILURE); 

    t_log* logger = log_create(LOGS_FILE, config_get_string_value(config, KEY_CLIENT_LOG), true, LOG_LEVEL_INFO);

    
    char* ip_kernel = config_get_string_value(config, KEY_IP_KERNEL);
    char* puerto_kernel = config_get_string_value(config, KEY_PUERTO_KERNEL);
    // char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    // char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);

    int conexion = crear_conexion(ip_kernel, puerto_kernel);
    log_info(logger, "Connected to Kernel -  SOCKET: %d", conexion);

    char* nombre_interfaz = "Interfaz1";

    enviar_mensaje(nombre_interfaz, conexion);

    t_io* io;
    op_code to_send;

    for (op_code cod_op = recibir_operacion(conexion); cod_op != -1; cod_op = recibir_operacion(conexion)){
        if (cod_op == IO){
            io = recibir_io_serializado(conexion, logger);

            if(io->type_instruction == IO_GEN_SLEEP) {
                int desplazamiento = 0;
                int size_sleep = 0;

                memcpy(&size_sleep, io->buffer + desplazamiento, sizeof(int));
                desplazamiento += sizeof(int);
                char* sleep_time = malloc(size_sleep);
                memcpy(sleep_time, io->buffer + desplazamiento, size_sleep);
                desplazamiento += size_sleep;

                to_send = desplazamiento != io->buffer_size ? IO_ERROR : IO_SUCCESS;
            

                if (to_send == IO_SUCCESS) {
                    log_info(logger, "Se solicita: %s - %s segundos", nombre_interfaz, sleep_time);
                    sleep(atoi(sleep_time));
                }

                free(sleep_time);
                free(io->buffer);
                free(io);

                log_info(logger, "Finalizada instruccion, se avisa a kernel");
                send(conexion, &to_send, sizeof(op_code), 0);
            }
        }
    }

   

	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);

    return EXIT_SUCCESS;
}