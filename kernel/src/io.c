#include "main.h"

void* io_main(void *arg) {
        log_info(logger, "Thread entrada salida creado");
    // void myiterator(char* value) {
    //     log_info(logger,"%s", value);
    // }

    // int server_fd = iniciar_servidor(config_get_string_value(config, KEY_PUERTO_ESCUCHA));
    // log_info(logger, "Server ready - SOCKET: %d", server_fd);

    // while(1) {
    //     int cliente_fd = esperar_cliente(server_fd);
    //     log_info(logger, "New client - SOCKET: %d", cliente_fd);

    //     t_list* lista;

    //     for (int cod_op = recibir_operacion(cliente_fd); cod_op != -1; cod_op = recibir_operacion(cliente_fd)){
    //         switch (cod_op) {
    //             case MENSAJE:
    //                 recibir_mensaje(cliente_fd, logger);
    //                 break;
    //             case PAQUETE:
    //                 lista = recibir_paquete(cliente_fd);
    //                 log_info(logger, "Me llegaron los siguientes valores:\n");
    //                 list_iterate(lista, (void*) myiterator);
    //                 break;
    //             default:
    //                 log_warning(logger,"Operacion desconocida. No quieras meter la pata");
    //                 break;
    //             }
    //     }

    //     log_error(logger, "Client disconnected - SOCKET: %d", cliente_fd);

    //     char* ip_cpu = config_get_string_value(config, KEY_IP_CPU);
    //     char* puerto_cpu_dispatch = config_get_string_value(config, KEY_PUERTO_CPU_DISPATCH);

    //     int conexion = crear_conexion(ip_cpu, puerto_cpu_dispatch);
    //     log_info(logger, "Connected to CPU - SOCKET: %d", conexion);
    //     // enviar_mensaje("Aca deberia mandar la lista obtenida del cliente, probando ahora la conexion", conexion);

    //     t_paquete * paquete = crear_paquete(PAQUETE);

    //     void mi_paquete_add(char * value) {
    //         agregar_a_paquete(paquete, value, strlen(value) + 1);
    //     }

    //     list_iterate(lista, (void *) mi_paquete_add);

    //     enviar_paquete(paquete, conexion);
    
    //     eliminar_paquete(paquete);
    //     liberar_conexion(conexion);
    // }
        
    // close(server_fd);
    return EXIT_SUCCESS;
}