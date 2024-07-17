#include "main.h"

void dialfs_io(char* nombre, t_config* config){
    t_log* logger = log_create("DIALFS.logs", config_get_string_value(config, KEY_LOGGER), true, LOG_LEVEL_INFO);

    char* fs_path = config_get_string_value(config, KEY_PATH_BASE_DIALFS);
    uint32_t block_count = (uint32_t) config_get_int_value(config, KEY_BLOCK_COUNT);
    uint32_t block_size = (uint32_t) config_get_int_value(config, KEY_BLOCK_SIZE);
    uint32_t retraso_compactacion = (uint32_t) config_get_int_value(config, KEY_RETRASO_COMPACTACION);

    uint32_t size_block_file = block_count * block_size;
    uint32_t size_bit_file = size_bit_file;

    char* bloques_name = string_duplicate(fs_path);
    char* bitmap_name = string_duplicate(fs_path);
    string_append(&bloques_name, "bloques.dat");
    string_append(&bitmap_name, "bitmap.dat");

    FILE* fd_bloques = fopen(bloques_name, "r+b");
    if (fd_bloques == NULL) fd_bloques = fopen(bloques_name, "w+b");

    FILE* fd_bitmap = fopen(bitmap_name, "r+b");
    if (fd_bitmap == NULL) fd_bitmap = fopen(bitmap_name, "w+b");
    
    fseek(fd_bitmap, 0, SEEK_END);
    int bit_size = ftell(fd_bitmap);
    void* bits = calloc(1, size_bit_file);
    
    log_info(logger, "bitzie: %i", bit_size);

    if (bit_size < size_bit_file) fwrite(bits + bit_size, size_bit_file - bit_size, 1, fd_bitmap);
    fseek(fd_bitmap, 0, SEEK_SET);

    free(bits);
    fclose(fd_bitmap);
    fclose(fd_bloques);

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

        op_code to_send = IO_ERROR;

    for (op_code cod_op = recibir_operacion(kernel_fd); cod_op != -1; cod_op = recibir_operacion(kernel_fd)){
        if (cod_op == IO){
            t_io* io = recibir_io_serializado(kernel_fd, logger);

            if(io->type_instruction == IO_FS_CREATE) {
                char* nombre_archivo = malloc(io->buffer_size);
                memcpy(nombre_archivo, io->buffer, io->buffer_size);

                log_info(logger, "Archivo %s crear", nombre_archivo);

                void* bits = calloc(1, size_bit_file);
                fread(bits, size_bit_file, 1, fd_bitmap);
                fseek(fd_bitmap, 0, SEEK_SET);
                mem_hexdump(bits, size_bit_file);

                t_bitarray* bit_map = bitarray_create_with_mode(bits, size_bit_file, MSB_FIRST);
                int posicion = -1;

                 for(int i = 0; i < bitarray_get_max_bit(bit_map); i += 1){
                    if(!bitarray_test_bit(bit_map, i)) {
                        bitarray_set_bit(bit_map, i);
                        posicion = i;
                        break;
                    }
                }

                mem_hexdump(bit_map->bitarray, bit_map->size);
                fwrite(bits, size_bit_file, 1, fd_bitmap);
                fseek(fd_bitmap, 0, SEEK_SET);

                free(bits);

                if (posicion > -1) {
                    char* info = string_new();
                    string_append(&info, "BLOQUE_INICIAL=");
                    string_append(&info, string_itoa(posicion));
                    string_append(&info, "\nTAMANIO_ARCHIVO=0");

                    FILE* fd_new;

                    char* path = string_duplicate(fs_path);
                    string_append(&path, nombre_archivo);
                    fd_new = fopen(path, "w");
                    fwrite(info, strlen(info) + 1, 1, fd_new);
                    fclose(fd_new);

                    free(path);
                    free(info);
                }

                usleep(unidad_trabajo * 1000);

                to_send = posicion > -1 ? IO_SUCCESS : IO_ERROR;

                bitarray_destroy(bit_map);
                free(nombre_archivo);
                free(io->buffer);
                free(io);
            }

            if(io->type_instruction == IO_FS_DELETE) {
                char* sleep_time = malloc(io->buffer_size);
                memcpy(sleep_time, io->buffer, io->buffer_size);

                log_info(logger, "Archivo %s borrar", sleep_time);
                usleep(unidad_trabajo * 1000);

                to_send = IO_SUCCESS;

                free(sleep_time);
                free(io->buffer);
                free(io);
            }

            if(io->type_instruction == IO_FS_TRUNCATE) {
                char* nombre_archivo;
                int tamaño_nombre;
                uint32_t valor_truncate;

                memcpy(&tamaño_nombre, io->buffer, sizeof(int));
                nombre_archivo = malloc(tamaño_nombre);
                memcpy(nombre_archivo, io->buffer + sizeof(int), tamaño_nombre);
                memcpy(&valor_truncate, io->buffer + sizeof(int) + tamaño_nombre, sizeof(uint32_t));

                log_info(logger, "Archivo - %s truncate %u", nombre_archivo, valor_truncate);
                usleep(unidad_trabajo * 1000);

                to_send = IO_SUCCESS;

                free(nombre_archivo);
                free(io->buffer);
                free(io);
            }

            if(io->type_instruction == IO_FS_READ){
                t_list* frames = list_create();
                uint32_t desplazamiento = 0;
                char* nombre_archivo;
                int tamaño_nombre;
                uint32_t comienzo_puntero;
                uint32_t tamaño_escribir;


                memcpy(&tamaño_nombre, io->buffer, sizeof(uint32_t));
                desplazamiento += sizeof(uint32_t);
                nombre_archivo = malloc(tamaño_nombre);

                memcpy(nombre_archivo, io->buffer + desplazamiento, tamaño_nombre);
                desplazamiento += tamaño_nombre;

                memcpy(&comienzo_puntero, io->buffer + desplazamiento, sizeof(uint32_t));
                desplazamiento += sizeof(uint32_t);

                memcpy(&tamaño_escribir, io->buffer + desplazamiento, sizeof(uint32_t));
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

                printf("Leer %s cantidad %u desde %u", nombre_archivo, tamaño_escribir, comienzo_puntero);

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

            if(io->type_instruction == IO_FS_WRITE){
                t_list* frames = list_create();
                uint32_t desplazamiento = 0;
                char* nombre_archivo;
                int tamaño_nombre;
                uint32_t comienzo_puntero;
                uint32_t tamaño_a_leer;


                memcpy(&tamaño_nombre, io->buffer, sizeof(uint32_t));
                desplazamiento += sizeof(uint32_t);
                nombre_archivo = malloc(tamaño_nombre);

                memcpy(nombre_archivo, io->buffer + desplazamiento, tamaño_nombre);
                desplazamiento += tamaño_nombre;

                memcpy(&comienzo_puntero, io->buffer + desplazamiento, sizeof(uint32_t));
                desplazamiento += sizeof(uint32_t);

                memcpy(&tamaño_a_leer, io->buffer + desplazamiento, sizeof(uint32_t));
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

                printf("Escribir %s cantidad %u desde %u\n", nombre_archivo, tamaño_a_leer, comienzo_puntero);
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

    fclose(fd_bloques);
    fclose(fd_bitmap);
}