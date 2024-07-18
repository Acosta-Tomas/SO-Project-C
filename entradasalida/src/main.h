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
#define TAMANIO_ARCHIVO "nTAMANIO_ARCHIVO"

typedef enum {
    GENERICA,
    STDIN,
    STDOUT,
    DIALFS
} interfaz;

extern t_log* logger;
extern t_config* config;

interfaz mapInterfaz (char*);
void *read_stdin(uint32_t);
void stdin_clear_buffer();

void generica_io(char*, t_config*);
void stdin_io(char*, t_config*);
void stdout_io(char*, t_config*);
void dialfs_io(char*, t_config*);

#endif

/*
    Me traigo todo el bloques.dat, bitmap.dat y los archivos a un config.
    Cada vez que hago una operacion, hago el write al donde corresponda.
    No hace falta leer cada vez porque vos manjeas los estados.
*/