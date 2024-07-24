#include "main.h"

uint32_t next_pid;
int memoria_fd;

void* consola_main(void *arg){
    log_debug(logger, "Thread consola creado");

    next_pid = 0;
	char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);
    memoria_fd = crear_conexion(ip_memoria, puerto_memoria);

    log_info(logger, "SOCKET: %d - Memoria", memoria_fd);

    while(1) {
        char* leido = readline("> ");
        if (leido) add_history(leido);
        if (!strncmp(leido, "exit", 4)) {
            free(leido);
            break;
        }

        char** command = string_split(leido, " ");

        free(leido);
        
        if (strcmp(command[0], "INICIAR_PROCESO") == 0) {
            iniciar_proceso(memoria_fd, command[0], command[1]);
            free(command);
            continue;
        }

        if (strcmp(command[0], "EJECUTAR_SCRIPT") == 0) {
            ejecutar_script(memoria_fd, command[0], command[1]);
            free(command);
            continue;
        }

        if (strcmp(command[0], "FINALIZAR_PROCESO") == 0) {
            if (command[1] == NULL) printf("Faltan argumentos, por favor intente nuevamente\n");
            else {
                finalizar_pid((uint32_t) atoi(command[1]));
                free(command[1]);
            };
            
            free(command[0]);
            free(command);
            continue;
        }

        if (strcmp(command[0], "DETENER_PLANIFICACION") == 0) {
            if (isStopped) printf("Ya se encuentra detenida la planificación\n");
            else {
                isStopped = true;
                sem_wait(&plani_run);
            }

            free(command[0]);   
            free(command);   
            continue;
        }

        if (strcmp(command[0], "INICIAR_PLANIFICACION") == 0) {
            if (isStopped) {
                isStopped = false;
                sem_post(&plani_run);
            } else printf("Ya esta corriendo la planificación\n");

            free(command[0]);
            free(command);
            continue;
        }

        if (strcmp(command[0], "MULTIPROGRAMACION") == 0) {
            if (command[1] == NULL) printf("Faltan argumentos, por favor intente nuevamente\n");
            else {
                cambiar_multiprogramacion((uint32_t) atoi(command[1]));
                free(command[1]);
            }

            free(command[0]);
            free(command);            
            continue;
        }

        if (strcmp(command[0], "PROCESO_ESTADO") == 0) {
            print_estados_procesos();
            free(command[0]);
            free(command);
            continue;
        }

        printf("Comando no reconocido");
    }

    liberar_conexion(memoria_fd);
   
   return EXIT_SUCCESS;
}

void cambiar_multiprogramacion(uint32_t new_multi){
    if (new_multi > multiprogramacion) {
        uint32_t cant_singals = new_multi - multiprogramacion;
        multiprogramacion = new_multi;
        
        for(int i = 0; i < cant_singals; i += 1) sem_post(&cont_multi);

        return;
    }

    if (new_multi < multiprogramacion) {
        uint32_t* cant_waits = malloc(sizeof(u_int32_t));
        *(cant_waits) = multiprogramacion - new_multi;
        multiprogramacion = new_multi;

        pthread_t wait_thread;
    
        if (pthread_create(&wait_thread, NULL, multi_change_waits, cant_waits)) {
            log_error(logger, "Wait threads no se pudo inicializar");
            exit(EXIT_FAILURE);
        } else pthread_detach(wait_thread);

        return;
    }
}

void* multi_change_waits(void* waits){
    uint32_t cant_waits = *(uint32_t*) waits;
    free(waits);

    for(int i = 0; i < cant_waits; i += 1) sem_wait(&cont_multi);
    
    return EXIT_SUCCESS;
}

void finalizar_pid(uint32_t pid){
    if ((int) pid == running_pid) {
        sem_wait(&mutex_interrupt);
        interrupt_pid->pid = (uint32_t) running_pid;
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
        finalizar_proceso(pcb_end, "INTERRUPTED_BY_USER");
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
            finalizar_proceso(pcb_end, "INTERRUPTED_BY_USER");
            list_destroy(recursos_list);
            return;
        }

        sem_post(&mutex_recurso);
    }

    list_destroy(recursos_list);

    t_list* io_list = dictionary_elements(dict_io_clients);

    bool is_pid_list_io(void* io) {
        t_io_queue* io_to_remove = (t_io_queue*) io;

        return io_to_remove->pcb->pid == pid;
    };

    for (int i = 0; i < list_size(io_list); i += 1){
        t_io_client* io_client = list_get(io_list, i);

        sem_wait(&io_client->mutex_io);
        t_io_queue* io = list_remove_by_condition(io_client->queue_io->elements, &is_pid_list_io);

        if (io != NULL) {
            pcb_end = io->pcb;
            sem_post(&io_client->mutex_io);
            sem_wait(&io_client->hay_io);
            finalizar_proceso(pcb_end, "INTERRUPTED_BY_USER");

            free(io->io_info->buffer);
            free(io->io_info);
            free(io);
            list_destroy(io_list);
            return;
        }

        sem_post(&io_client->mutex_io);
    }

    list_destroy(io_list);

    printf("PID: %u\n no existe o no se pudo finalizar", pid);
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
            free(leido);
            free(leido_splitted);
        }

        free(comandos);
        free(script);

    } else printf("Script no se pudo inicializar\n");

    free(comando);
    free(archivo);
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

    free(comando);
    free(archivo);
}

void enviar_new(){
    t_pcb* pid_context = crear_context(next_pid);

    log_info(logger, "Se crea el proceso %u en NEW", next_pid);

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

void print_pid(void* pcb) {
    t_pcb* pcb_print = (t_pcb*) pcb;

    printf("%u, ", pcb_print->pid); 
}

void print_io_pd(void *io) {
    t_io_queue* io_print = (t_io_queue*) io;

    printf("%u, ", io_print->pcb->pid); 
}

void print_io(char* key, void* io){
     t_io_client* io_print = (t_io_client*) io;

     printf("\tIO - %s: ", key);
     list_iterate(io_print->queue_io->elements, &print_io_pd);
}

void print_recursos(char* key, void* recurso){
     t_recursos* recurso_print = (t_recursos*) recurso;

     printf("\tWAIT - %s: ", key);
     list_iterate(recurso_print->queue_waiting->elements, &print_pid);
}

void print_estados_procesos(void){
    printf("NEW: ");
    list_iterate(queue_new->elements, &print_pid);
    printf("\nREADY - PRIORITY: ");
    list_iterate(queue_priority_ready->elements, &print_pid);
    printf("\nREADY: ");
    list_iterate(queue_ready->elements, &print_pid);
    printf("\nEXEC: ");
    if (running_pid >= 0) printf("%i\n", running_pid);
    printf("\nBLOCKED:\n");
    dictionary_iterator(dict_io_clients, &print_io);
    dictionary_iterator(dict_recursos, &print_recursos);
}