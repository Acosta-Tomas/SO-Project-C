#ifndef MAIN_H_
#define MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <utils/server.h>

// FILES
#define CONFIG_FILE "memoria.config"
#define LOGS_FILE "memoria.logs"

// KEYS_CONFIG_FILE
#define KEY_CLIENT_LOG "LOGGER_CLIENTE"
#define KEY_SERVER_LOG "LOGGER_SERVIDOR"
#define KEY_PUERTO_ESCUCHA "PUERTO_ESCUCHA"

extern t_log* logger;
extern t_config* config;

#endif