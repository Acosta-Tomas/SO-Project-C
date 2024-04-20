#ifndef MAIN_H_
#define MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <utils/server.h>

// FILES
#define CONFIG_FILE "kernel.config"
#define LOGS_FILE "kernel.logs"

// KEYS_CONFIG_FILE
#define KEY_CLIENT_LOG "LOGGER_CLIENTE"
#define KEY_SERVER_LOG "LOGGER_SERVIDOR"
#define KEY_PUERTO_ESCUCHA "PUERTO_ESCUCHA"
#define KEY_IP_CPU "IP_CPU"
#define KEY_PUERTO_CPU_DISPATCH "PUERTO_CPU_DISPATCH"
#define KEY_PUERTO_CPU_INTERRUPT "PUERTO_CPU_INTERRUPT"
#define KEY_IP_MEMORIA "IP_MEMORIA"
#define KEY_PUERTO_MEMORIA "PUERTO_MEMORIA"

extern t_config* config;

void servidor_main(void);
void cliente_main(void);

#endif