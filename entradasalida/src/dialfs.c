#include "main.h"


FILE* open_file(char* nombre){
    FILE* file = fopen(nombre, "r+b");    // Si existe lo abro

    if (file == NULL) file = fopen(nombre, "w+b"); // Si no existe lo creo 

    return file;
}

uint32_t get_file_size(FILE* file){
    fseek(file, 0, SEEK_END);
    uint32_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    return file_size;
}

void* get_bloques(FILE* file, uint32_t size){
    void* buffer = calloc(1, size);
    uint32_t file_size = get_file_size(file);

    uint32_t size_to_read = file_size >= size ? size : file_size;

    fread(buffer, size_to_read, 1, file);
    fseek(file, 0, SEEK_SET);
    mem_hexdump(buffer, size);

    return buffer;
}

void update_bloques(FILE* file, void* bloques, uint32_t size_block_file){
    fseek(file, 0, SEEK_SET);
    fwrite(bloques, size_block_file, 1, file);
    fseek(file, 0, SEEK_SET);
}

t_bitarray* get_bitmap(FILE* file, uint32_t size){
    void* bits = get_bloques(file, size);

    t_bitarray* bit_map = bitarray_create_with_mode(bits, size, MSB_FIRST);

    return bit_map;
}

void update_bitmap(FILE* file, t_bitarray* bitmap, uint32_t size_bit_file){
    update_bloques(file, bitmap->bitarray, size_bit_file);
}

bool is_metadata_file(struct dirent* entry) {
    if (entry->d_type != DT_REG) return false;

    if (string_ends_with(entry->d_name, ".dat")) return false;

    return true;
}

t_list* get_metadata(char* path){
    t_list* metadata = list_create();
    struct dirent *entry;

    DIR *directory = opendir(path);

    while ((entry = readdir(directory)) != NULL){
        printf("%s - %i\n", entry->d_name, is_metadata_file(entry));
        if (is_metadata_file(entry)){
            char* file_name = string_duplicate(path);
            string_append(&file_name, entry->d_name);

            t_config *file = config_create(file_name);

            if (file == NULL) continue;

            list_add(metadata, file);
        }
    }

    free(entry);
    closedir(directory);

    return metadata;
}

int get_init_block_file(t_bitarray* bitmap, uint32_t size){

    int posicion = -1;
    int bitmap_size = bitarray_get_max_bit(bitmap);

    for(int i = 0; i < bitmap_size; i += 1){
        if(!bitarray_test_bit(bitmap, i)) {
            int flag = true;
            int k = i + 1;

            while (k - i < size && k < bitmap_size && flag) {
                if(bitarray_test_bit(bitmap, i)) flag = false;
                k += 1;
            }

            if (flag) {
                posicion = i;
                break;
            } 

            i = k;
        }
    }

    return posicion;
}

t_config* get_config(t_list* config_list, char* path){
    bool file_in_list(void* cfg) {
        t_config* file_cfg = (t_config*) cfg;

        return !strcmp(file_cfg->path, path);
    };

    return list_find(config_list, &file_in_list);
}

t_config* remove_config(t_list* config_list, char* path){
    bool file_in_list(void* cfg) {
        t_config* file_cfg = (t_config*) cfg;

        return !strcmp(file_cfg->path, path);
    };

    return list_remove_by_condition(config_list, &file_in_list);
}

op_code create_file(t_list* metadata, char* nombre, int bloque_inicial){
    t_config* new_file_cfg = get_config(metadata, nombre);

    if (new_file_cfg == NULL) return IO_ERROR;

    char* info = string_new();
    string_append(&info, "BLOQUE_INICIAL=");
    string_append(&info, string_itoa(bloque_inicial));
    string_append(&info, "\nTAMANIO_ARCHIVO=0");

    FILE* new_file = fopen(nombre, "w");
    fwrite(info, strlen(info) + 1, 1, new_file);
    fclose(new_file);

    new_file_cfg = config_create(nombre);
    list_add(metadata, new_file_cfg);

    free(info);

    return IO_SUCCESS;
}

void dialfs_io(char* nombre, t_config* config){
    t_log* logger = log_create("DIALFS.logs", config_get_string_value(config, KEY_LOGGER), true, LOG_LEVEL_INFO);

    char* fs_path = config_get_string_value(config, KEY_PATH_BASE_DIALFS);
    uint32_t block_count = (uint32_t) config_get_int_value(config, KEY_BLOCK_COUNT);
    uint32_t block_size = (uint32_t) config_get_int_value(config, KEY_BLOCK_SIZE);
    uint32_t retraso_compactacion = (uint32_t) config_get_int_value(config, KEY_RETRASO_COMPACTACION);

    uint32_t size_block_file = block_count * block_size;
    uint32_t size_bit_file = block_count/8;

    char* bloques_name = string_duplicate(fs_path);
    char* bitmap_name = string_duplicate(fs_path);
    string_append(&bloques_name, "bloques.dat");
    string_append(&bitmap_name, "bitmap.dat");

    FILE* fd_bloques = open_file(bloques_name);
    FILE* fd_bitmap = open_file(bitmap_name);

    void* bloques = get_bloques(fd_bloques, size_block_file);
    t_bitarray* bitmap = get_bitmap(fd_bitmap, size_bit_file);
    t_list* metadata = get_metadata(fs_path);

    return;

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

            switch (io->type_instruction){
                case IO_FS_CREATE:
                    char* file_path_create = get_io_file(io, fs_path);
                    log_info(logger, "Archivo %s crear", file_path_create);

                    int init_block = get_init_block_file(bitmap, 1);

                    if (init_block > -1) to_send = create_file(metadata, file_path_create, init_block);
                    else to_send = IO_ERROR;

                    if (to_send == IO_SUCCESS) {
                        bitarray_set_bit(bitmap, init_block);
                        update_bitmap(fd_bitmap, bitmap, size_bit_file);
                    }

                    free(file_path_create);
                    free(io->buffer);
                    free(io);
                    break;
                    
                case IO_FS_DELETE:
                    to_send = IO_SUCCESS;
                    char* file_path_delete = get_io_file(io, fs_path);
                    log_info(logger, "Archivo %s borrar", file_path_delete);
                    
                    t_config* file_cfg = remove_config(metadata, file_path_delete);

                    if (file_cfg != NULL) {
                        int init_bloque = config_get_int_value(file_cfg, "BLOQUE_INICIAL");
                        remove(file_path_delete);

                        bitarray_clean_bit(bitmap, init_bloque);
                        update_bitmap(fd_bitmap, bitmap, size_bit_file);

                        config_destroy(file_cfg);
                    }

                    free(file_path_delete);
                    free(io->buffer);
                    free(io);
                    break;
                
                case IO_FS_TRUNCATE:
                    break;
                
                case IO_FS_READ:
                    break;

                case IO_FS_WRITE:
                    break;
        
                default:
                    log_info(logger, "Instruccion no valida, se avisa a kernel");
                    break;

                usleep(unidad_trabajo * 1000);
                send(kernel_fd, &to_send, sizeof(op_code), 0);
                log_info(logger, "Finalizada instruccion, se avisa a kernel");
            }

            // if(io->type_instruction == IO_FS_TRUNCATE) {
            //     char* nombre_archivo;
            //     int tamaño_nombre;
            //     uint32_t valor_truncate;

            //     memcpy(&tamaño_nombre, io->buffer, sizeof(int));
            //     nombre_archivo = malloc(tamaño_nombre);
            //     memcpy(nombre_archivo, io->buffer + sizeof(int), tamaño_nombre);
            //     memcpy(&valor_truncate, io->buffer + sizeof(int) + tamaño_nombre, sizeof(uint32_t));

            //     log_info(logger, "Archivo - %s truncate %u", nombre_archivo, valor_truncate);
            //     usleep(unidad_trabajo * 1000);

            //     to_send = IO_SUCCESS;

            //     free(nombre_archivo);
            //     free(io->buffer);
            //     free(io);
            // }

            // if(io->type_instruction == IO_FS_READ){
            //     t_list* frames = list_create();
            //     uint32_t desplazamiento = 0;
            //     char* nombre_archivo;
            //     int tamaño_nombre;
            //     uint32_t comienzo_puntero;
            //     uint32_t tamaño_escribir;


            //     memcpy(&tamaño_nombre, io->buffer, sizeof(uint32_t));
            //     desplazamiento += sizeof(uint32_t);
            //     nombre_archivo = malloc(tamaño_nombre);

            //     memcpy(nombre_archivo, io->buffer + desplazamiento, tamaño_nombre);
            //     desplazamiento += tamaño_nombre;

            //     memcpy(&comienzo_puntero, io->buffer + desplazamiento, sizeof(uint32_t));
            //     desplazamiento += sizeof(uint32_t);

            //     memcpy(&tamaño_escribir, io->buffer + desplazamiento, sizeof(uint32_t));
            //     desplazamiento += sizeof(uint32_t);

            //     while(desplazamiento < io->buffer_size){
            //         t_memoria_fisica* frame = malloc(sizeof(t_memoria_fisica));

            //         memcpy(&frame->direccion_fisica, io->buffer + desplazamiento, sizeof(uint32_t));
            //         desplazamiento += sizeof(uint32_t);

            //         memcpy(&frame->bytes, io->buffer + desplazamiento, sizeof(uint32_t));
            //         desplazamiento += sizeof(uint32_t);

            //         list_add(frames, frame);
            //     }

            //     char* buffer = malloc(tamaño_escribir + 1);

            //     printf("Leer %s cantidad %u desde %u", nombre_archivo, tamaño_escribir, comienzo_puntero);

            //     if (fgets(buffer, tamaño_escribir + 1, stdin) != NULL) {
            //         if (buffer[strlen(buffer) - 1] != '\n') stdin_clear_buffer();
                
            //         printf("Usted ingresó: %s - %ld\n", (char*) buffer, strlen(buffer));
            //     }

            //     int status = escribir_memoria(memoria_fd, buffer, frames);
                
            //     to_send = status == -1 ? IO_ERROR : IO_SUCCESS;

            //     free(io->buffer);
            //     free(io);
            //     free(buffer);
            //     list_destroy(frames);
            // }

            // if(io->type_instruction == IO_FS_WRITE){
            //     t_list* frames = list_create();
            //     uint32_t desplazamiento = 0;
            //     char* nombre_archivo;
            //     int tamaño_nombre;
            //     uint32_t comienzo_puntero;
            //     uint32_t tamaño_a_leer;


            //     memcpy(&tamaño_nombre, io->buffer, sizeof(uint32_t));
            //     desplazamiento += sizeof(uint32_t);
            //     nombre_archivo = malloc(tamaño_nombre);

            //     memcpy(nombre_archivo, io->buffer + desplazamiento, tamaño_nombre);
            //     desplazamiento += tamaño_nombre;

            //     memcpy(&comienzo_puntero, io->buffer + desplazamiento, sizeof(uint32_t));
            //     desplazamiento += sizeof(uint32_t);

            //     memcpy(&tamaño_a_leer, io->buffer + desplazamiento, sizeof(uint32_t));
            //     desplazamiento += sizeof(uint32_t);

            //     while(desplazamiento < io->buffer_size){
            //         t_memoria_fisica* frame = malloc(sizeof(t_memoria_fisica));

            //         memcpy(&frame->direccion_fisica, io->buffer + desplazamiento, sizeof(uint32_t));
            //         desplazamiento += sizeof(uint32_t);

            //         memcpy(&frame->bytes, io->buffer + desplazamiento, sizeof(uint32_t));
            //         desplazamiento += sizeof(uint32_t);

            //         list_add(frames, frame);
            //     }

            //     char* buffer = (char*) malloc(tamaño_a_leer);

            //     int status = leer_memoria(memoria_fd, buffer, frames);
                
            //     to_send = status == -1 ? IO_ERROR : IO_SUCCESS;

            //     printf("Escribir %s cantidad %u desde %u\n", nombre_archivo, tamaño_a_leer, comienzo_puntero);
            //     printf("Datos leidos: %.*s\n", tamaño_a_leer, buffer);

            //     free(io->buffer);
            //     free(io);
            //     free(buffer);
            //     list_destroy(frames);
            // }
        }
    }

    fclose(fd_bitmap);
    fclose(fd_bloques);
}