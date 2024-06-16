#include "main.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <nombre> <edad>\n", argv[0]);
        return 1;
    }

    t_config* config = config_create(CONFIG_FILE);

    if (config == NULL) exit(EXIT_FAILURE); 

    t_log* logger = log_create(LOGS_FILE, config_get_string_value(config, KEY_CLIENT_LOG), true, LOG_LEVEL_INFO);

    char* ip_kernel = config_get_string_value(config, KEY_IP_KERNEL);
    char* puerto_kernel = config_get_string_value(config, KEY_PUERTO_KERNEL);
    char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);

    int kernel_fd = crear_conexion(ip_kernel, puerto_kernel);
    log_info(logger, "Connected to Kernel -  SOCKET: %d", kernel_fd);

    int memoria_fd = crear_conexion(ip_memoria, puerto_memoria);
    log_info(logger, "Connected to Memoria -  SOCKET: %d", memoria_fd);

    char* nombre_interfaz = argv[1];

    enviar_mensaje(nombre_interfaz, kernel_fd);

    t_io* io;
    op_code to_send = IO_ERROR;

    for (op_code cod_op = recibir_operacion(kernel_fd); cod_op != -1; cod_op = recibir_operacion(kernel_fd)){
        if (cod_op == IO){
            io = recibir_io_serializado(kernel_fd, logger);

            if(io->type_instruction == IO_GEN_SLEEP) {
                char* sleep_time = malloc(io->buffer_size);
                memcpy(sleep_time, io->buffer, io->buffer_size);

                log_info(logger, "Se solicita: %s - %s segundos", nombre_interfaz, sleep_time);
                sleep(atoi(sleep_time));

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

                void* buffer = malloc(sizeof(tamaño_escribir));

                printf("Ingresar datos a guardar: ");

                if (fgets(buffer, tamaño_escribir, stdin) != NULL) {
                    printf("Usted ingresó: %s - %ld\n", (char*) buffer, strlen(buffer) + 1);
                }

                to_send = escribir_memoria(memoria_fd, buffer, frames);

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

                void* buffer = malloc(sizeof(tamaño_a_leer));

                to_send = leer_memoria(memoria_fd, buffer, frames);

                printf("Datos leidos: %s - %ld\n", (char*) buffer, strlen(buffer) + 1);

                free(buffer);
                list_destroy(frames);
            }

            log_info(logger, "Finalizada instruccion, se avisa a kernel");
            send(kernel_fd, &to_send, sizeof(op_code), 0);
        }
    }

   

	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(kernel_fd);
    liberar_conexion(memoria_fd);

    return EXIT_SUCCESS;
}

op_code escribir_memoria(int memoria_fd, void* buffer, t_list* frames){
    int offset_buffer = 0;

    while(list_size(frames)){
        t_memoria_fisica* frame = list_remove(frames, 0);
        t_paquete* paquete = crear_paquete(MEM_WRITE);
        op_code code;

        agregar_uint_a_paquete(paquete, &frame->direccion_fisica, sizeof(uint32_t));
        agregar_a_paquete(paquete, buffer + offset_buffer, frame->bytes);
        enviar_paquete(paquete, memoria_fd);
        eliminar_paquete(paquete);

        offset_buffer += frame->bytes;
        free(frame);

        recv(memoria_fd, &code, sizeof(op_code), MSG_WAITALL);
        if (code != MEM_SUCCESS) return IO_ERROR;
    }

    return IO_SUCCESS;
}

op_code leer_memoria(int memoria_fd, void* buffer, t_list* frames){
    int offset_buffer = 0;
    op_code code_op = MEM_READ;

    while(list_size(frames)){
        t_memoria_fisica* frame = list_remove(frames, 0);
        op_code code;

        send(memoria_fd, &code_op, sizeof(op_code), 0);
        send(memoria_fd, &frame->direccion_fisica, sizeof(uint32_t), 0);
        send(memoria_fd, &frame->bytes, sizeof(uint32_t), 0);

        recv(memoria_fd, &code, sizeof(op_code), MSG_WAITALL);
        recv(memoria_fd, buffer + offset_buffer, frame->bytes, MSG_WAITALL);

        offset_buffer += frame->bytes;
        free(frame);

        if (code != MEM_SUCCESS) return IO_ERROR;
    }

    return IO_SUCCESS;
}