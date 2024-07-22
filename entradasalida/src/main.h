#ifndef MAIN_H_
#define MAIN_H_

#include <utils/io_protocol.h>
#include <commons/bitarray.h>
#include <commons/memory.h>
#include <dirent.h>

// KEYS_CONFIG_FILE
#define KEY_TIPO_INTERFAZ "TIPO_INTERFAZ"
#define KEY_IP_KERNEL "IP_KERNEL"
#define KEY_PUERTO_KERNEL "PUERTO_KERNEL"
#define KEY_IP_MEMORIA "IP_MEMORIA"
#define KEY_PUERTO_MEMORIA "PUERTO_MEMORIA"
#define KEY_TIPO_INTERFAZ "TIPO_INTERFAZ"
#define KEY_UNIDAD_TRABAJO "TIEMPO_UNIDAD_TRABAJO"
#define KEY_PATH_BASE_DIALFS "PATH_BASE_DIALFS"
#define KEY_BLOCK_SIZE "BLOCK_SIZE"
#define KEY_BLOCK_COUNT "BLOCK_COUNT"
#define KEY_RETRASO_COMPACTACION "RETRASO_COMPACTACION"
#define KEY_LOGGER "LOGGER"

#define BLOQUE_INICIAL "BLOQUE_INICIAL"
#define TAMANIO_ARCHIVO "TAMANIO_ARCHIVO"

typedef enum {
    GENERICA,
    STDIN,
    STDOUT,
    DIALFS
} interfaz;

typedef struct {
    void* bloques;
    FILE* file;
    uint32_t file_size;
    uint32_t bloques_size;
    uint32_t compactacion_delay;
} t_bloques;

typedef struct {
    t_bitarray* bitmap;
    FILE* file;
    uint32_t file_size;
} t_bitmap;

extern t_log* logger;
extern t_config* config;

void generica_io(char*, t_config*);
void stdin_io(char*, t_config*);
void stdout_io(char*, t_config*);
void dialfs_io(char*, t_config*);
op_code fs_create_file(t_io*, t_list*, t_bitmap*, char*);
op_code fs_read_file(t_io*, t_list*, t_bloques*, char*, int);
op_code fs_write_file(t_io*, t_list*, t_bloques*, char*, int);
op_code fs_delete_file(t_io*, t_list*, t_bitmap*, t_bloques*, char*);
op_code fs_truncate_file(t_io*, t_list*, t_bitmap*, t_bloques*, char*);

void *read_stdin(uint32_t);
void stdin_clear_buffer();
interfaz mapInterfaz (char*);
FILE* open_file(char*);
uint32_t get_file_size(FILE*);
void* get_bloques(FILE*, uint32_t);
t_bitarray* get_bitmap(FILE*, uint32_t);
void update_file(FILE*, void*, uint32_t);
void update_bitmap(t_bitmap*);
void update_bloques(t_bloques*);
bool is_metadata_file(struct dirent*);
t_list* get_metadata(char*);
int get_init_block_file(t_bitarray* , uint32_t);
t_config* get_config(t_list*, char*);
t_config* remove_config(t_list*, char*);
op_code create_file(t_list*, char*, int);
void resize_down(t_bitarray*, int, int);
bool check_space_contiguo(t_bitarray*, int, int);
bool check_space_total(t_bitarray*, int);
void resize_up(t_bitarray*, int, int);
void realocate_data(void*, uint32_t, uint32_t, uint32_t);
bool sory_by_init_block(void*, void*);
void realocate_compactacion(t_config*, t_bitmap*, t_bloques*, int);

#endif

/*
    Me traigo todo el bloques.dat, bitmap.dat y los archivos a un config.
    Cada vez que hago una operacion, hago el write al donde corresponda.
    No hace falta leer cada vez porque vos manjeas los estados.
*/