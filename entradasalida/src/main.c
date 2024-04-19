#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>

void paquete(int);

int main(int argc, char* argv[]) {
    decir_hola("una Interfaz de Entrada/Salida");
    t_config* config = config_create("entradasalida.config");

    if (config == NULL) exit(EXIT_FAILURE); 

    t_log* logger = log_create("entradasalida.logs", config_get_string_value(config, "LOGGER_CLIENTE"), true, LOG_LEVEL_INFO);

    int conexion;
    char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char* puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

    // Revisar pporque siempre me tira warnings con esto!
    log_info(logger, ip_kernel);
    log_info(logger, puerto_kernel);
    log_info(logger, ip_memoria);
    log_info(logger, puerto_memoria);

    conexion = crear_conexion(ip_kernel, puerto_kernel);
    log_info(logger, "Me conecto al kernel para enviar paquetes");
    paquete(conexion);

    log_destroy(logger);
    config_destroy(config);
	liberar_conexion(conexion);

    return EXIT_SUCCESS;
}

void paquete(int conexion)
{
	// Ahora toca lo divertido!
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
	// ¡No te olvides de liberar las líneas y el paquete antes de regresar!
}
