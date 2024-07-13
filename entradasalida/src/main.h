#ifndef MAIN_H_
#define MAIN_H_

#include <utils/io_protocol.h>
#include <commons/bitarray.h>
#include <commons/memory.h>

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

extern t_log* logger;
extern t_config* config;

void stdin_clear_buffer();

#endif

/*
    Revisar STDOUT que impriome caracteres de mas, me llegan al buffer tambien de memoria
    (por ahi hay que armar paquete para esta operacion)
    Agregar separacion de Operaciones Permitidas por tipo de interfaz
*/