#include "main.h"

t_log* logger;

void dialfs_io(char* nombre, t_config* config){
    logger = log_create("DIALFS.logs", config_get_string_value(config, KEY_LOGGER), true, LOG_LEVEL_INFO);

    char* fs_path = config_get_string_value(config, KEY_PATH_BASE_DIALFS);
    uint32_t block_count = (uint32_t) config_get_int_value(config, KEY_BLOCK_COUNT);
    uint32_t block_size = (uint32_t) config_get_int_value(config, KEY_BLOCK_SIZE);

    t_bloques* bloques = malloc(sizeof(t_bloques));
    t_bitmap* bitmap = malloc(sizeof(t_bitmap));

    bloques->compactacion_delay = (uint32_t) config_get_int_value(config, KEY_RETRASO_COMPACTACION) * 1000;
    bloques->file_size = block_count * block_size;
    bloques->bloques_size = block_size;
    bitmap->file_size = block_count/8;

    char* bloques_name = string_duplicate(fs_path);
    char* bitmap_name = string_duplicate(fs_path);
    string_append(&bloques_name, "bloques.dat");
    string_append(&bitmap_name, "bitmap.dat");

    bloques->file = open_file(bloques_name);
    bitmap->file = open_file(bitmap_name);

    bloques->bloques = get_bloques(bloques->file, bloques->file_size);
    bitmap->bitmap = get_bitmap(bitmap->file, bitmap->file_size);
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
                case IO_FS_CREATE: to_send = fs_create_file(io, metadata, bitmap, fs_path); break;
                    
                case IO_FS_DELETE: to_send = fs_delete_file(io, metadata, bitmap, bloques, fs_path); break;
                
                case IO_FS_TRUNCATE: to_send = fs_truncate_file(io, metadata, bitmap, bloques, fs_path); break;
                
                case IO_FS_WRITE: to_send = fs_write_file(io, metadata, bloques, fs_path, memoria_fd); break;

                case IO_FS_READ: to_send = fs_read_file(io, metadata, bloques, fs_path, memoria_fd); break;
        
                default: log_warning(logger, "Instruccion no valida, se avisa a kernel"); break;
            }

            usleep(unidad_trabajo * 1000);
            send(kernel_fd, &to_send, sizeof(op_code), 0);
            log_info(logger, "Finalizada instruccion, se avisa a kernel");
        }
    }

    list_destroy(metadata);
    bitarray_destroy(bitmap->bitmap);

    fclose(bloques->file);
    fclose(bitmap->file);

    free(bloques->bloques);
    free(bloques);
    free(bitmap);
}

op_code fs_create_file(t_io* io, t_list* metadata, t_bitmap* bitmap , char* fs_path){
    char* file_path = get_io_file(io, fs_path);
    log_info(logger, "Archivo %s crear", file_path);

    int init_block = get_init_block_file(bitmap->bitmap, 1);

    if (init_block == -1) {
        free(file_path);
        return IO_ERROR;
    }

    op_code status = create_file(metadata, file_path, init_block);

    if (status == IO_SUCCESS) {
        bitarray_set_bit(bitmap->bitmap, init_block);
        update_bitmap(bitmap);
    }

    free(file_path);
    return status;
}

op_code fs_delete_file(t_io* io, t_list* metadata, t_bitmap* bitmap, t_bloques* bloques, char* fs_path){
    char* file_path = get_io_file(io, fs_path);
    log_info(logger, "Archivo %s borrar", file_path);
    
    t_config* file_cfg = remove_config(metadata, file_path);

    if (file_cfg == NULL) {
        free(file_path);
        return IO_ERROR;
    }

    int init_bloque = config_get_int_value(file_cfg, BLOQUE_INICIAL);
    int size = config_get_int_value(file_cfg, TAMANIO_ARCHIVO);
    size = (int) ceil((double)size / bloques->bloques_size);

    remove(file_path);

    if (size == 0 && init_bloque > -1) size = 1;
    resize_down(bitmap->bitmap, init_bloque, init_bloque + size);
    update_bitmap(bitmap);

    config_destroy(file_cfg);
    free(file_path);

    return IO_SUCCESS;
}

op_code fs_write_file(t_io* io, t_list* metadata, t_bloques* bloques, char* fs_path, int memoria_fd) {
    op_code status = IO_ERROR;
    t_fs_rw* fs_info_w = malloc(sizeof(t_fs_rw));
    char* file_path = get_io_read_write(io, fs_path, fs_info_w);

    if (file_path) {
        log_info(logger, "Archivo %s write", file_path);

        t_config* file_cfg = get_config(metadata, file_path);

        char* buffer = (char*) malloc(fs_info_w->size);

        if(leer_memoria(memoria_fd, buffer, fs_info_w->frames) > -1 && file_cfg != NULL) {
            int bloque = config_get_int_value(file_cfg, BLOQUE_INICIAL);

            memcpy(bloques->bloques + bloque * bloques->bloques_size + fs_info_w->ptr, buffer, fs_info_w->size);
            update_bloques(bloques);
            status = IO_SUCCESS;
        }

        free(buffer);
    }

    list_destroy(fs_info_w->frames);
    free(file_path);
    free(fs_info_w);

    return status;
}

op_code fs_read_file(t_io* io, t_list* metadata, t_bloques* bloques, char* fs_path, int memoria_fd){
    op_code status = IO_ERROR;
    t_fs_rw* fs_info_r = malloc(sizeof(t_fs_rw));
    char* file_path = get_io_read_write(io, fs_path, fs_info_r);

    if (file_path) {
        log_info(logger, "Archivo %s write", file_path);

        t_config* file_cfg = get_config(metadata, file_path);

        if(file_cfg != NULL) {
            int bloque = config_get_int_value(file_cfg, BLOQUE_INICIAL);
            char* buffer = (char*) malloc(fs_info_r->size);

            memcpy(buffer, bloques->bloques + bloque * bloques->bloques_size + fs_info_r->ptr, fs_info_r->size);

            status = escribir_memoria(memoria_fd, buffer, fs_info_r->frames) > -1 ? IO_SUCCESS : IO_ERROR;

            free(buffer);
        }
        
    }

    list_destroy(fs_info_r->frames);
    free(file_path);
    free(fs_info_r);

    return status;
}

op_code fs_truncate_file(t_io* io, t_list* metadata, t_bitmap* bitmap, t_bloques* bloques, char* fs_path){
    int new_size;
    char* file_path = get_io_truncate(io, fs_path, &new_size);

    if (file_path) {
        log_info(logger, "Archivo %s truncate size %i", file_path, new_size);
        t_config* file_cfg = get_config(metadata, file_path);

        if(file_cfg == NULL) {
            free(file_path);
            return IO_ERROR;
        };

        int file_size = config_get_int_value(file_cfg, TAMANIO_ARCHIVO);
        int init_block = config_get_int_value(file_cfg, BLOQUE_INICIAL);
        int current_size = (int) ceil((double)file_size / bloques->bloques_size);
        new_size = (int) ceil((double)new_size / bloques->bloques_size);

        if (current_size == 0 && init_block > -1) current_size = 1;

        if (new_size == current_size) {
            config_set_value(file_cfg, TAMANIO_ARCHIVO, string_itoa(new_size * bloques->bloques_size));
            config_save(file_cfg);
            
            free(file_path);
            return IO_SUCCESS;
        }

        if (new_size < current_size) {
            resize_down(bitmap->bitmap, init_block + new_size, init_block + current_size);
            config_set_value(file_cfg, TAMANIO_ARCHIVO, string_itoa(new_size * bloques->bloques_size));
            if (!new_size) config_set_value(file_cfg, BLOQUE_INICIAL, string_itoa(-1));

            config_save(file_cfg);
            update_bitmap(bitmap);

            free(file_path);
            return IO_SUCCESS;
        }

        if (new_size > current_size) {
            bool tiene_espacio = check_space_contiguo(bitmap->bitmap, init_block + current_size, init_block + new_size);

            if (tiene_espacio) {
                resize_up(bitmap->bitmap, init_block + current_size, init_block + new_size);
                config_set_value(file_cfg, TAMANIO_ARCHIVO, string_itoa(new_size * bloques->bloques_size));

                update_bitmap(bitmap);
                config_save(file_cfg);

                free(file_path);
                return IO_SUCCESS;
            }

            int new_init_block = get_init_block_file(bitmap->bitmap, new_size);

            if (new_init_block > -1) {
                resize_down(bitmap->bitmap, init_block, init_block + current_size);
                resize_up(bitmap->bitmap, new_init_block, new_init_block + new_size);
                realocate_data(bloques->bloques, init_block, new_init_block, current_size * bloques->bloques_size);

                config_set_value(file_cfg, BLOQUE_INICIAL, string_itoa(new_init_block));
                config_set_value(file_cfg, TAMANIO_ARCHIVO, string_itoa(new_size * bloques->bloques_size));

                update_bloques(bloques);
                update_bitmap(bitmap);
                config_save(file_cfg);

                free(file_path);
                return IO_SUCCESS;

            }


            if (check_space_total(bitmap->bitmap, new_size)) {
                log_info(logger, "Compactando...");

                list_sort(metadata, &sory_by_init_block);
                resize_down(bitmap->bitmap, 0, bitarray_get_max_bit(bitmap->bitmap));

                for(int i = 0; i < list_size(metadata); i += 1){
                    t_config* cfg = list_get(metadata, i);
                    
                    if (strcmp(file_cfg->path, cfg->path)){
                        realocate_compactacion(cfg, bitmap, bloques, 0);
                    }
                }

                realocate_compactacion(file_cfg, bitmap, bloques, new_size);

                update_bitmap(bitmap);
                update_bloques(bloques);
                
                usleep(bloques->compactacion_delay);
                
                free(file_path);
                return IO_SUCCESS;

            }
        }

        free(file_path);
        return new_size == current_size ? IO_SUCCESS : IO_ERROR;
    }

    free(file_path);
    return IO_ERROR;
}
