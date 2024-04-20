#include "main.h"

int main(int argc, char* argv[]) {
    t_config* config = config_create(CONFIG_FILE);

    if (config == NULL) exit(EXIT_FAILURE); 

    t_log* logger = log_create(LOGS_FILE, config_get_string_value(config, KEY_CLIENT_LOG), true, LOG_LEVEL_INFO);

    int conexion;
    char* ip_kernel = config_get_string_value(config, KEY_IP_KERNEL);
    char* puerto_kernel = config_get_string_value(config, KEY_PUERTO_KERNEL);
    char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);

    conexion = crear_conexion(ip_kernel, puerto_kernel);

    log_info(logger, "Connected to Kernel -  SOCKET: %d", conexion);

    paquete_por_consola(conexion);

    terminar_programa(conexion, logger, config);

    return EXIT_SUCCESS;
}

void paquete_por_consola(int conexion){
	char* leido;
	t_paquete* paquete;

	paquete = crear_paquete();

	for (leido = readline("> "); leido && strcmp(leido, ""); leido = readline("> ")){
		agregar_a_paquete(paquete, leido, strlen(leido) + 1);
		free(leido);
	}
	
	if (leido != NULL) free(leido);

	enviar_paquete(paquete, conexion);
	eliminar_paquete(paquete);
}