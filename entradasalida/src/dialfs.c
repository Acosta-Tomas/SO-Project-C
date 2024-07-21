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
                if(bitarray_test_bit(bitmap, k)) flag = false;
                k += 1;
            }

            if (flag && k - i == size) {
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

    if (new_file_cfg != NULL) return IO_ERROR;

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

void resize_down(t_bitarray* bitmap, int init_bit, int end_bit){
    for (int i = init_bit; i < end_bit; i += 1)
        bitarray_clean_bit(bitmap, i);
}

bool check_space_contiguo(t_bitarray* bitmap, int init_bit, int end_bit){
    if (init_bit < 0) return false;
    for (int i = init_bit; i < end_bit; i += 1){
        if(bitarray_get_max_bit(bitmap) == i || bitarray_test_bit(bitmap, i)) return false;
    }

    return true;
}

bool check_space_total(t_bitarray* bitmap, int cant_bits){
    for (int i = 0; i < bitarray_get_max_bit(bitmap); i += 1){
        if(!bitarray_test_bit(bitmap, i)) cant_bits -= 1;
    }

    return cant_bits < 1;
}

void resize_up(t_bitarray* bitmap, int init_bit, int end_bit){
    for (int i = init_bit; i < end_bit; i += 1)
        bitarray_set_bit(bitmap, i);
}

void realocate_data(void* buffer, uint32_t current_pos, uint32_t new_pos, uint32_t size){
    memcpy(buffer + new_pos, buffer + current_pos, size);
}

bool sory_by_init_block(void* config1, void* config2){
    t_config* cfg1 = (t_config*) config1;
    t_config* cfg2 = (t_config*) config2;

    int bloque_cfg1 = config_get_int_value(cfg1, BLOQUE_INICIAL);
    int bloque_cfg2 = config_get_int_value(cfg2, BLOQUE_INICIAL);

    return bloque_cfg1 < bloque_cfg2;
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
                    break;
                    
                case IO_FS_DELETE:
                    to_send = IO_SUCCESS;
                    char* file_path_delete = get_io_file(io, fs_path);
                    log_info(logger, "Archivo %s borrar", file_path_delete);
                    
                    t_config* file_cfg = remove_config(metadata, file_path_delete);

                    if (file_cfg != NULL) {
                        int init_bloque = config_get_int_value(file_cfg, BLOQUE_INICIAL);
                        int size = config_get_int_value(file_cfg, TAMANIO_ARCHIVO);
                        size = (int) ceil((double)size / block_size);
                        remove(file_path_delete);

                        if (size == 0 && init_block > -1) size = 1;
                        resize_down(bitmap, init_bloque, init_bloque + size);
                        update_bitmap(fd_bitmap, bitmap, size_bit_file);

                        config_destroy(file_cfg);
                    }

                    free(file_path_delete);
                    break;
                
                case IO_FS_TRUNCATE: 
                    int new_size;
                    char* file_truncate = get_io_truncate(io, fs_path, &new_size);
                    to_send = IO_ERROR;

                    if (file_truncate) {
                        log_info(logger, "Archivo %s truncate size %i", file_truncate, new_size);
                        t_config* file_cfg = get_config(metadata, file_truncate);

                        if(file_cfg == NULL) {
                            free(file_truncate);
                            break;
                        };

                        int current_size = config_get_int_value(file_cfg, TAMANIO_ARCHIVO);
                        int init_block = config_get_int_value(file_cfg, BLOQUE_INICIAL);
                        current_size = (int) ceil((double)current_size / block_size);
                        new_size = (int) ceil((double)new_size / block_size);
                        if (current_size == 0 && init_block > -1) current_size = 1;

                        if (new_size == current_size) {
                            config_set_value(file_cfg, TAMANIO_ARCHIVO, string_itoa(new_size * block_size));
                            config_save(file_cfg);

                            to_send = IO_SUCCESS;
                            free(file_truncate);
                            break;
                        }

                        if (new_size < current_size) {
                            resize_down(bitmap, init_block + new_size, init_block + current_size);
                            config_set_value(file_cfg, TAMANIO_ARCHIVO, string_itoa(new_size * block_size));
                            if (!new_size) config_set_value(file_cfg, BLOQUE_INICIAL, string_itoa(-1));

                            config_save(file_cfg);
                            update_bitmap(fd_bitmap, bitmap, size_bit_file);

                            to_send = IO_SUCCESS;
                            free(file_truncate);
                            break;
                        }

                        if (new_size > current_size) {
                            bool tiene_espacio = check_space_contiguo(bitmap, init_block + current_size, init_block + new_size);

                            if (tiene_espacio) {
                                resize_up(bitmap, init_block + current_size, init_block + new_size);
                                config_set_value(file_cfg, TAMANIO_ARCHIVO, string_itoa(new_size * block_size));

                                update_bitmap(fd_bitmap, bitmap, size_bit_file);
                                config_save(file_cfg);

                                to_send = IO_SUCCESS;
                                free(file_truncate);
                                break;
                            }

                            int new_init_block = get_init_block_file(bitmap, new_size);

                            if (new_init_block > -1) {
                                resize_down(bitmap, init_block, init_block + current_size);
                                resize_up(bitmap, new_init_block, new_init_block + new_size);
                                realocate_data(bloques, init_block, new_init_block, current_size * block_size);
                                config_set_value(file_cfg, BLOQUE_INICIAL, string_itoa(new_init_block));
                                config_set_value(file_cfg, TAMANIO_ARCHIVO, string_itoa(new_size * block_size));

                                update_bloques(fd_bloques, bloques, size_block_file);
                                update_bitmap(fd_bitmap, bitmap, size_bit_file);
                                config_save(file_cfg);

                                to_send = IO_SUCCESS;
                                free(file_truncate);
                                break;
                            }

                            if (check_space_total(bitmap, new_size)) {
                                list_sort(metadata, &sory_by_init_block);
                                resize_down(bitmap, 0, bitarray_get_max_bit(bitmap));

                                for(int i = 0; i < list_size(metadata); i += 1){
                                    t_config* cfg = list_get(metadata, i);
                                    bool isCfgTruncate = !strcmp(file_cfg->path, cfg->path);
                                    int init_block = config_get_int_value(cfg, BLOQUE_INICIAL);
                                    int size = config_get_int_value(cfg, TAMANIO_ARCHIVO);
                                    int bit_size = isCfgTruncate ? new_size : (int) ceil((double)size / block_size);
                                    int new_block = get_init_block_file(bitmap, bit_size);

                                    resize_up(bitmap, new_block, new_block + bit_size);

                                    void* buffer = malloc(size);
                                    memcpy(buffer, bloques + init_block * block_size, size);
                                    memcpy(bloques + init_block * block_size, buffer, size);

                                    config_set_value(cfg, BLOQUE_INICIAL, string_itoa(new_block));
                                    if(isCfgTruncate) config_set_value(cfg, TAMANIO_ARCHIVO, string_itoa(bit_size * block_size));
                                    config_save(cfg);
                                    free(buffer);
                                }

                                update_bitmap(fd_bitmap, bitmap, size_bit_file);
                                update_bloques(fd_bloques, bloques, size_block_file);
                                usleep(retraso_compactacion * 1000);
                                to_send = IO_SUCCESS;
                                free(file_truncate);
                                break;

                            }
                        }

                    }
                    
                    break;
                
                case IO_FS_WRITE: 
                    to_send = IO_ERROR;
                    t_fs_rw* fs_info_w = malloc(sizeof(t_fs_rw));
                    char* file_write = get_io_read_write(io, fs_path, fs_info_w);

                    if (file_write) {
                        log_info(logger, "Archivo %s write", file_write);

                        t_config* file_cfg = get_config(metadata, file_write);
                        char* buffer = (char*) malloc(fs_info_w->size);
                        int status = leer_memoria(memoria_fd, buffer, fs_info_w->frames);

                        if(file_cfg != NULL && status > -1) {
                            int bloque = config_get_int_value(file_cfg, BLOQUE_INICIAL);

                            memcpy(bloques + bloque * block_size + fs_info_w->ptr, buffer, fs_info_w->size);
                            update_bloques(fd_bloques, bloques, size_block_file);
                            to_send = IO_SUCCESS;
                        }

                        list_destroy(fs_info_w->frames);
                        free(file_write);
                        free(fs_info_w);
                        free(buffer);
                    }

                    break;

                case IO_FS_READ: to_send = IO_ERROR;
                    to_send = IO_ERROR;
                    t_fs_rw* fs_info_r = malloc(sizeof(t_fs_rw));
                    char* file_read = get_io_read_write(io, fs_path, fs_info_r);

                    if (file_write) {
                        log_info(logger, "Archivo %s write", file_read);

                        t_config* file_cfg = get_config(metadata, file_read);
                        char* buffer = (char*) malloc(fs_info_r->size);

                        if(file_cfg != NULL) {
                            int bloque = config_get_int_value(file_cfg, BLOQUE_INICIAL);

                            memcpy(buffer, bloques + bloque * block_size + fs_info_r->ptr, fs_info_r->size);
                            
                            int status = escribir_memoria(memoria_fd, buffer, fs_info_r->frames);
                            if (status > -1 ) to_send = IO_SUCCESS;
                        }

                        list_destroy(fs_info_r->frames);
                        free(file_read);
                        free(fs_info_r);
                        free(buffer);
                    }

                    break;
        
                default:
                    log_info(logger, "Instruccion no valida, se avisa a kernel");
                    break;
            }

        
            usleep(unidad_trabajo * 1000);
            send(kernel_fd, &to_send, sizeof(op_code), 0);
            log_info(logger, "Finalizada instruccion, se avisa a kernel");
        }
    }

    fclose(fd_bitmap);
    fclose(fd_bloques);
}