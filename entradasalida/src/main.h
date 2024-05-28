#ifndef MAIN_H_
#define MAIN_H_

#include <utils/io_protocol.h>

// FILES
#define CONFIG_FILE "entradasalida.config"
#define LOGS_FILE "entradasalida.logs"

// KEYS_CONFIG_FILE
#define KEY_CLIENT_LOG "LOGGER_CLIENTE"
#define KEY_IP_KERNEL "IP_KERNEL"
#define KEY_PUERTO_KERNEL "PUERTO_KERNEL"
#define KEY_IP_MEMORIA "IP_MEMORIA"
#define KEY_PUERTO_MEMORIA "PUERTO_MEMORIA"

extern t_log* logger;
extern t_config* config;

#endif