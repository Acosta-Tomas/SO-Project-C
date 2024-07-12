#include "main.h"

uint32_t next_pid;
int memoria_fd;

void* consola_main(void *arg){
    log_info(logger, "Thread consola creado");

    next_pid = 0;
	char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);
    memoria_fd = crear_conexion(ip_memoria, puerto_memoria);

    log_info(logger, "Connected to Memoria - SOCKET: %d", memoria_fd);

    while(1) {
        char* leido = readline("> ");
        char** command = string_split(leido, " ");

        free(leido);
        
        if (strcmp(command[0], "INICIAR_PROCESO") == 0) {
            iniciar_proceso(memoria_fd, command[0], command[1]);
            continue;
        }

        if (strcmp(command[0], "EJECUTAR_SCRIPT") == 0) {
            ejecutar_script(memoria_fd, command[0], command[1]);
            continue;
        }

        if (strcmp(command[0], "FINALIZAR_PROCESO") == 0) {
            if (command[1] == NULL) printf("Faltan argumentos, por favor intente nuevamente\n");
            else finalizar_pid((uint32_t) atoi(command[1]));

            continue;
        }

        if (strcmp(command[0], "DETENER_PLANIFICACION") == 0) {
            if (isStopped) printf("Ya se encuentra detenida la planificación\n");
            else {
                isStopped = true;
                sem_wait(&plani_run);
            }

            continue;
        }

        if (strcmp(command[0], "INICIAR_PLANIFICACION") == 0) {
            if (isStopped) {
                isStopped = false;
                sem_post(&plani_run);
            } else printf("Ya esta corriendo la planificación\n");

            continue;
        }

        if (strcmp(command[0], "MULTIPROGRAMACION") == 0) {
            if (command[1] == NULL) printf("Faltan argumentos, por favor intente nuevamente\n");
            else cambiar_multiprogramacion((uint32_t) atoi(command[1]));
            continue;
        }

        printf("Comando no reconocido, escriba help para ver los comandos disponibles\n");
    }

    liberar_conexion(memoria_fd);
   
   return EXIT_SUCCESS;
}

void cambiar_multiprogramacion(uint32_t new_multi){
    if (new_multi > multiprogramacion) {
        uint32_t cant_singals = new_multi - multiprogramacion;
        multiprogramacion = new_multi;

        log_info(logger, "Signals: %u", cant_singals);
        
        for(int i = 0; i < cant_singals; i += 1) sem_post(&cont_multi);

        return;
    }

    if (new_multi < multiprogramacion) {
        uint32_t cant_waits = multiprogramacion - new_multi;
        multiprogramacion = new_multi;

        pthread_t wait_thread;
    
        if (pthread_create(&wait_thread, NULL, multi_change_waits, &cant_waits)) {
            log_error(logger, "Wait threads no se pudo inicializar");
            exit(EXIT_FAILURE);
        } else pthread_detach(wait_thread);

        return;
    }
}

void* multi_change_waits(void* waits){
    uint32_t cant_waits = *(uint32_t*) waits;

    log_info(logger, "Waits: %u", cant_waits);

    for(int i = 0; i < cant_waits; i += 1) sem_wait(&cont_multi);

    return EXIT_SUCCESS;
}

void finalizar_pid(uint32_t pid){
    if (pid == running_pid) {
        sem_wait(&mutex_interrupt);
        interrupt_pid->pid = running_pid;
        interrupt_pid->type_interrupt = END_PID_USER;
        sem_post(&mutex_interrupt);
        sem_post(&hay_interrupt);

        return;
    }
    
    t_pcb* pcb_end = NULL;

    bool is_pid_list(void* pcb) {
        t_pcb* pcb_to_remove = (t_pcb*) pcb;

        return pcb_to_remove->pid == pid;
    };

    sem_wait(&mutex_ready);
    pcb_end = list_remove_by_condition(queue_ready->elements, &is_pid_list);
    if(pcb_end == NULL) pcb_end = list_remove_by_condition(queue_priority_ready->elements, &is_pid_list);
    sem_post(&mutex_ready);

    if (pcb_end != NULL) {
        log_info(logger, "Proceso Finalizado por usuario - PID: %u", pcb_end->pid);
        log_registers(pcb_end, logger);
        finalizar_proceso(pcb_end);
        sem_wait(&hay_ready);

        return;
    }

    t_list* recursos_list = dictionary_elements(dict_recursos);

    for (int i = 0; i < list_size(recursos_list); i += 1) {
        t_recursos* recurso = list_get(recursos_list, i);

        sem_wait(&mutex_recurso);
        pcb_end = list_remove_by_condition(recurso->queue_waiting->elements, &is_pid_list);

        if (pcb_end != NULL) {
            recurso->cant_instancias += 1;
            sem_post(&mutex_recurso);
            finalizar_proceso(pcb_end);
            list_destroy(recursos_list);
            return;
        }

        sem_post(&mutex_recurso);

    }

    printf("No se encontro proceso: %u\n", pid);
}

void ejecutar_script(int conexion, char* comando, char* archivo){
    if (archivo == NULL) {
        printf("Faltan argumentos, por favor intente nuevamente\n");
        return;
    }

    enviar_mensaje(archivo, conexion, INIT_SCRIPT);

    op_code codigo = recibir_operacion(conexion);

    if (codigo == INIT_SCRIPT_SUCCESS) {
        char* script = recibir_mensaje(conexion);

        char** comandos = string_split(script, "\n");

        for(int i = 0; i < string_array_size(comandos); i += 1){
            char* leido = *(comandos + i);
            char** leido_splitted = string_split(leido, " ");

            iniciar_proceso(conexion, leido_splitted[0], leido_splitted[1]);
        }

    } else printf("Script no se pudo inicializar\n");
    
}

void iniciar_proceso(int conexion, char* comando, char* archivo){
    if (archivo == NULL) {
        printf("Faltan argumentos, por favor intente nuevamente\n");
        return;
    }

    t_paquete* paquete = crear_paquete(INIT_PID);
    agregar_init_process_paquete(paquete, next_pid, archivo);

    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);

    op_code estado = recibir_operacion(conexion);

    if (estado == INIT_PID_SUCCESS) {
        enviar_new();
        printf("Proceso %s correctamente inicializado\n", archivo);
    }

    if (estado == INIT_PID_ERROR) printf("Proceso %s no se pudo inicializar\n", archivo);

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
    new_context->quantum = quantum;
    new_context->status = NEW;
    new_context->pid = pid;
    new_context->registers = calloc(1, sizeof(t_registros));
    new_context->recursos = string_new();

    new_context->registers->pc = 0;

    return new_context;
}