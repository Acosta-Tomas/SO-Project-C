#include "main.h"

uint32_t next_pid;

void* consola_main(void *arg){
    log_info(logger, "Thread consola creado");

    next_pid = 0;
	char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);
    int conexion = crear_conexion(ip_memoria, puerto_memoria);

    log_info(logger, "Connected to Memoria - SOCKET: %d", conexion);

    printf("Ecribir INICIAR_PROCESO y su path para comenzar un programa\n");

    while(1) {
        char* leido = readline("> ");
        char** command = string_split(leido, " ");

        free(leido);

        if (strcmp(command[0], "INICIAR_PROCESO") == 0) {
            printf("Inciando proceso, por favor espere...\n");
            op_code estado = iniciar_proceso(conexion, command[0], command[1]);

            if (estado == INIT_PID_SUCCESS) {
                enviar_new();
                printf("Inciado correctamente\n");
            }
            if (estado == INIT_PID_ERROR) printf("No se pudo iniciar el proceso\n");
        } else  printf("Comando no reconocido, intente nuevamente\n");

    }
   
   return EXIT_SUCCESS;
}


op_code iniciar_proceso(int conexion, char* comando, char* archivo){
    if (archivo == NULL) return INIT_PID_ERROR;

    t_paquete* paquete = crear_paquete(INIT_PID);
    log_info(logger, "Comando: %s", comando);
    log_info(logger, "Archivo: %s", archivo);

    agregar_a_paquete(paquete, comando, strlen(comando) + 1);
    agregar_a_paquete(paquete, archivo, strlen(comando) + 1);

    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);

    return recibir_operacion(conexion);

}

void enviar_new(){
    t_pcb* pid_context = crear_context(next_pid);

    sem_wait(&mutex_new);
    queue_push(queue_new, pid_context);
    sem_post(&mutex_new);
    sem_post(&hay_new);

    next_pid++;
}

t_pcb* crear_context(uint32_t pid){
    t_pcb* new_context = malloc(sizeof(t_pcb));

    new_context->pc = 0;
    new_context->status = NEW;
    new_context->pid = pid;
    new_context->registers = malloc(sizeof(t_registros));

    new_context->registers->pc = 0;

    return new_context;
}