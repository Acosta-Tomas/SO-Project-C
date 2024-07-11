#include "main.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Uso: %s <nombre_interfaz> <config_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    t_config* config = config_create(argv[2]);
    char* nombre_interfaz = argv[1];

    if (config == NULL) exit(EXIT_FAILURE); 

    char* tipo_interfaz = config_get_string_value(config, KEY_TIPO_INTERFAZ);
    char* file_logs = string_duplicate(tipo_interfaz);
    string_append(&file_logs, (char*) ".logs");

    t_log* logger = log_create(file_logs, config_get_string_value(config, KEY_LOGGER), true, LOG_LEVEL_INFO);

    char* ip_kernel = config_get_string_value(config, KEY_IP_KERNEL);
    char* puerto_kernel = config_get_string_value(config, KEY_PUERTO_KERNEL);
    char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);
    int unidad_trabajo = config_get_int_value(config, KEY_UNIDAD_TRABAJO);

    int kernel_fd = crear_conexion(ip_kernel, puerto_kernel);
    log_info(logger, "Connected to Kernel -  SOCKET: %d", kernel_fd);

    int memoria_fd = crear_conexion(ip_memoria, puerto_memoria);
    log_info(logger, "Connected to Memoria -  SOCKET: %d", memoria_fd);

    enviar_mensaje(nombre_interfaz, kernel_fd, MENSAJE);

    op_code to_send = IO_ERROR;

    for (op_code cod_op = recibir_operacion(kernel_fd); cod_op != -1; cod_op = recibir_operacion(kernel_fd)){
        if (cod_op == IO){
            t_io* io = recibir_io_serializado(kernel_fd, logger);

            if(io->type_instruction == IO_GEN_SLEEP) {
                char* sleep_time = malloc(io->buffer_size);
                memcpy(sleep_time, io->buffer, io->buffer_size);

                log_info(logger, "Se solicita: %s - %s segundos", nombre_interfaz, sleep_time);
                usleep(atoi(sleep_time) * unidad_trabajo * 1000);

                to_send = IO_SUCCESS;

                free(sleep_time);
                free(io->buffer);
                free(io);
            }

            if(io->type_instruction == IO_STDIN_READ){
                t_list* frames = list_create();
                uint32_t desplazamiento = 0;
                uint32_t tamaño_escribir;

                memcpy(&tamaño_escribir, io->buffer, sizeof(uint32_t));
                desplazamiento += sizeof(uint32_t);

                while(desplazamiento < io->buffer_size){
                    t_memoria_fisica* frame = malloc(sizeof(t_memoria_fisica));

                    memcpy(&frame->direccion_fisica, io->buffer + desplazamiento, sizeof(uint32_t));
                    desplazamiento += sizeof(uint32_t);

                    memcpy(&frame->bytes, io->buffer + desplazamiento, sizeof(uint32_t));
                    desplazamiento += sizeof(uint32_t);

                    list_add(frames, frame);
                }

                char* buffer = malloc(tamaño_escribir + 1);

                printf("Ingresar datos a guardar: ");

                if (fgets(buffer, tamaño_escribir + 1, stdin) != NULL) {
                    if (buffer[strlen(buffer) - 1] != '\n') stdin_clear_buffer();
                
                    printf("Usted ingresó: %s - %ld\n", (char*) buffer, strlen(buffer));
                }

                int status = escribir_memoria(memoria_fd, buffer, frames);
                
                to_send = status == -1 ? IO_ERROR : IO_SUCCESS;

                free(io->buffer);
                free(io);
                free(buffer);
                list_destroy(frames);
            }

            if(io->type_instruction == IO_STDOUT_WRITE){
                t_list* frames = list_create();
                uint32_t desplazamiento = 0;
                uint32_t tamaño_a_leer;

                memcpy(&tamaño_a_leer, io->buffer, sizeof(uint32_t));
                desplazamiento += sizeof(uint32_t);

                while(desplazamiento < io->buffer_size){
                    t_memoria_fisica* frame = malloc(sizeof(t_memoria_fisica));

                    memcpy(&frame->direccion_fisica, io->buffer + desplazamiento, sizeof(uint32_t));
                    desplazamiento += sizeof(uint32_t);

                    memcpy(&frame->bytes, io->buffer + desplazamiento, sizeof(uint32_t));
                    desplazamiento += sizeof(uint32_t);

                    list_add(frames, frame);
                }

                char* buffer = (char*) malloc(tamaño_a_leer);

                int status = leer_memoria(memoria_fd, buffer, frames);
                
                to_send = status == -1 ? IO_ERROR : IO_SUCCESS;

                printf("Datos leidos: %.*s\n", tamaño_a_leer, buffer);

                free(io->buffer);
                free(io);
                free(buffer);
                list_destroy(frames);
            }

            send(kernel_fd, &to_send, sizeof(op_code), 0);
            log_info(logger, "Finalizada instruccion, se avisa a kernel");
        }
    }

   

	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(kernel_fd);
    liberar_conexion(memoria_fd);

    return EXIT_SUCCESS;
}

void stdin_clear_buffer() {
    char c = fgetc(stdin);
    while (c != '\n' && c != EOF) {
        printf("%c", c);
        c = fgetc(stdin);
    }
}