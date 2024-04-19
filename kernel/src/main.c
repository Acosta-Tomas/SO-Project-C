#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>

void servidor_main(t_config*);
void cliente_main(t_config*);

int main(int argc, char* argv[]) {
    t_config* config = config_create("kernel.config");

    if (config == NULL) exit(EXIT_FAILURE); 

    decir_hola("Kernel");
    
    pthread_t  serveridor_thread, cliente_thread;

    // Crear el hilo del servidor
    if (pthread_create(&serveridor_thread, NULL, (void*) servidor_main, config)) {
        perror("Error creating server thread");
        exit(EXIT_FAILURE);
    }

    // Esperar un poco para que el servidor esté listo antes de iniciar el cliente
    sleep(1);

    // Crear el hilo del cliente
    if (pthread_create(&cliente_thread, NULL, (void*) cliente_main, config)) {
        perror("Error creating client thread");
        exit(EXIT_FAILURE);
    }

    // Esperar a que ambos hilos terminen
    pthread_join(serveridor_thread, NULL);
    pthread_join(cliente_thread, NULL);

    config_destroy(config);
    return EXIT_SUCCESS;
}

// Probando poder crear un server que escuche como el tp0 en un thread (esta funcion la pasaria a otro archivo server.c);
void servidor_main(t_config* config) {
    t_log* logger = log_create("kernel.logs", config_get_string_value(config, "LOGGER_SERVIDOR"), true, LOG_LEVEL_DEBUG);

    void myiterator(char* value) {
        log_info(logger,"%s", value);
    }

    log_info(logger, "Thread Servidor creado exitosamente");

    int server_fd = iniciar_servidor(config_get_string_value(config, "PUERTO_ESCUCHA"));
    log_info(logger, "Servidor listo para recibir al cliente");

    int cliente_fd = esperar_cliente(server_fd);
    log_info(logger, "Se conecto un cliente");

    t_list* lista;

    for (int cod_op = recibir_operacion(cliente_fd); cod_op != -1; cod_op = recibir_operacion(cliente_fd)){
        switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(cliente_fd, logger);
                break;
            case PAQUETE:
                lista = recibir_paquete(cliente_fd);
                log_info(logger, "Me llegaron los siguientes valores:\n");
                list_iterate(lista, (void*) myiterator);
                break;
            default:
                log_warning(logger,"Operacion desconocida. No quieras meter la pata");
                break;
		}
    }

    log_error(logger, "el cliente se desconecto, servidor terminado");
    close(server_fd);

    
    char* ip_cpu = config_get_string_value(config, "IP_CPU");
    char* puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    int conexion = crear_conexion(ip_cpu, puerto_cpu_dispatch);
	log_info(logger, "Me conecto a CPU para enviar los paquetes recibidos de mi cliente");
    enviar_mensaje("Aca deberia mandar la lista obtenida del cliente, probando ahora la conexion", conexion);
    // paquete(conexion);
    liberar_conexion(conexion);
    // while (1) {
	// 	int cod_op = recibir_operacion(cliente_fd);
	// 	switch (cod_op) {
	// 	case MENSAJE:
	// 		recibir_mensaje(cliente_fd, logger);
	// 		break;
	// 	case PAQUETE:
	// 		lista = recibir_paquete(cliente_fd);
	// 		log_info(logger, "Me llegaron los siguientes valores:\n");
	// 		list_iterate(lista, (void*) myiterator);
	// 		break;
	// 	case -1:
	// 		log_error(logger, "el cliente se desconecto. Terminando servidor");
    //         log_destroy(logger);
	// 		return;
	// 	default:
	// 		log_warning(logger,"Operacion desconocida. No quieras meter la pata");
	// 		break;
	// 	}
	// }
    log_destroy(logger);
}

// Probando poder crear un cliente que mande info como el tp0 en un thread (esta funcion la pasaria a otro archivo cliente.c);
void cliente_main(t_config* config){
    t_log* logger = log_create("kernel.logs", config_get_string_value(config, "LOGGER_CLIENTE"), true, LOG_LEVEL_INFO);
    log_info(logger, "Thread Cliente creado exitosamente");

    char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    char* ip_cpu = config_get_string_value(config, "IP_CPU");
    char* puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    char* puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");

    // Revisar pporque siempre me tira warnings con esto!
    log_info(logger, ip_memoria);
    log_info(logger, puerto_memoria);
    log_info(logger, ip_cpu);
    log_info(logger, puerto_cpu_dispatch);
    log_info(logger, puerto_cpu_interrupt);

    leer_consola(logger);

    log_destroy(logger);
}