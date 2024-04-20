#include "main.h"

t_config* config;

int main(int argc, char* argv[]) {
    config = config_create(CONFIG_FILE);

    if (config == NULL) exit(EXIT_FAILURE); 
    
    pthread_t serveridor_thread, cliente_thread; // los threads fueron para probar, se usa solo el servidor 

    // Crear el hilo del servidor
    if (pthread_create(&serveridor_thread, NULL, (void*) servidor_main, NULL)) {
        perror("Error creating server thread");
        exit(EXIT_FAILURE);
    }

    // Esperar un poco para que el servidor esté listo antes de iniciar el cliente
    sleep(1);

    // Crear el hilo del cliente
    if (pthread_create(&cliente_thread, NULL, (void*) cliente_main, NULL)) {
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
void servidor_main() {
    t_log* logger = log_create(LOGS_FILE, config_get_string_value(config, KEY_SERVER_LOG), true, LOG_LEVEL_DEBUG);

    void myiterator(char* value) {
        log_info(logger,"%s", value);
    }

    int server_fd = iniciar_servidor(config_get_string_value(config, KEY_PUERTO_ESCUCHA));
    log_info(logger, "Server ready - SOCKET: %d", server_fd);

    while(1) {
        int cliente_fd = esperar_cliente(server_fd);
        log_info(logger, "New client - SOCKET: %d", cliente_fd);

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

        log_error(logger, "Client disconnected - SOCKET: %d", cliente_fd);

        char* ip_cpu = config_get_string_value(config, KEY_IP_CPU);
        char* puerto_cpu_dispatch = config_get_string_value(config, KEY_PUERTO_CPU_DISPATCH);

        int conexion = crear_conexion(ip_cpu, puerto_cpu_dispatch);
        log_info(logger, "Connected to CPU - SOCKET: %d", conexion);
        // enviar_mensaje("Aca deberia mandar la lista obtenida del cliente, probando ahora la conexion", conexion);

        t_paquete * paquete = crear_paquete();

        void mi_paquete_add(char * value) {
            agregar_a_paquete(paquete, value, strlen(value) + 1);
        }

        list_iterate(lista, (void *) mi_paquete_add);

        enviar_paquete(paquete, conexion);
    
        eliminar_paquete(paquete);
        liberar_conexion(conexion);
    }
        
    log_destroy(logger);
    close(server_fd);
}

// Solo para probar threads, se incia y termina al toque
void cliente_main(){
    t_log* logger = log_create(LOGS_FILE, config_get_string_value(config, KEY_CLIENT_LOG), true, LOG_LEVEL_INFO);
    log_info(logger, "Thread - Client for testing");

    char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);
    char* ip_cpu = config_get_string_value(config, KEY_IP_CPU);
    char* puerto_cpu_dispatch = config_get_string_value(config, KEY_PUERTO_CPU_DISPATCH);
    char* puerto_cpu_interrupt = config_get_string_value(config, KEY_PUERTO_CPU_INTERRUPT);

    log_destroy(logger);
}