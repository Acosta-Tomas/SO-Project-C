#include "main.h"

interfaz mapInterfaz (char* interfaz) {
    if (strcmp(interfaz, "STDIN") == 0) return STDIN;
    if (strcmp(interfaz, "STDOUT") == 0) return STDOUT;
    if (strcmp(interfaz, "GENERICA") == 0) return GENERICA;
    if (strcmp(interfaz, "DIALFS") == 0) return DIALFS;
    return GENERICA;
}

void *read_stdin(uint32_t size){
    char* buffer = malloc(size + 1);

    printf("Ingresar datos: ");

    if (fgets(buffer, size + 1, stdin) != NULL) {
        if (buffer[strlen(buffer) - 1] != '\n') stdin_clear_buffer();
    
        printf("Usted ingresó: %s - %ld\n", (char*) buffer, strlen(buffer));
    }

    return buffer;
}

void stdin_clear_buffer() {
    char c = fgetc(stdin);
    while (c != '\n' && c != EOF) {
        printf("%c", c);
        c = fgetc(stdin);
    }
}

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

t_bitarray* get_bitmap(FILE* file, uint32_t size){
    void* bits = get_bloques(file, size);

    t_bitarray* bit_map = bitarray_create_with_mode(bits, size, MSB_FIRST);

    return bit_map;
}

void update_file(FILE* file, void* buffer, uint32_t buffer_size){
    fseek(file, 0, SEEK_SET);
    fwrite(buffer, buffer_size, 1, file);
    fseek(file, 0, SEEK_SET);
}

void update_bitmap(t_bitmap* bitmap){
    update_file(bitmap->file, bitmap->bitmap->bitarray, bitmap->file_size);
}

void update_bloques(t_bloques* bloques){
    update_file(bloques->file, bloques->bloques, bloques->file_size);
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

// new_size sirve para el realocar el archivo que hacemos truncate, en el resto no hay que mandar eso (es decir mandamos 0 para que sea falso)
void realocate_compactacion(t_config* file_cfg, t_bitmap* bitmap, t_bloques* bloques, int new_size){
    int current_block = config_get_int_value(file_cfg, BLOQUE_INICIAL);
    int file_size = config_get_int_value(file_cfg, TAMANIO_ARCHIVO);
    int bit_size = new_size ? new_size : (int) ceil((double)file_size / bloques->bloques_size);

    int new_block = get_init_block_file(bitmap->bitmap, bit_size);
    resize_up(bitmap->bitmap, new_block, new_block + bit_size);

    void* buffer = malloc(file_size);
    memcpy(buffer, bloques->bloques + current_block * bloques->bloques_size, file_size);
    memcpy(bloques->bloques + new_block * bloques->bloques_size, buffer, file_size);

    if (new_size) config_set_value(file_cfg, TAMANIO_ARCHIVO, string_itoa(new_size * bloques->bloques_size));
    config_set_value(file_cfg, BLOQUE_INICIAL, string_itoa(new_block));
    config_save(file_cfg);
    free(buffer);
}