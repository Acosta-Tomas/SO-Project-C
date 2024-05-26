#include "main.h"

t_log* logger;
t_config* config;
t_pcb* pcb;

pthread_t thread_dispatch, thread_interrupt;

bool has_interrupt = false;

sem_t mutex_interrupt;
// sem_t mutex_pcb;

int main(int argc, char* argv[]) {
    config = config_create(CONFIG_FILE);

    if (config == NULL) exit(EXIT_FAILURE); 

    logger = log_create(LOGS_FILE, config_get_string_value(config, KEY_SERVER_LOG), true, LOG_LEVEL_DEBUG);

    sem_init(&mutex_interrupt, 0, 1);
    // sem_init(&mutex_pcb, 0, 1);

    if (pthread_create(&thread_dispatch, NULL, dispatch, NULL)) {
        log_error(logger, "Problema al crear hilo para el servidor dispatch");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&thread_interrupt, NULL, interrupt, NULL)) {
        log_error(logger, "Problema al crear hilo para el servidor interrupt");
        exit(EXIT_FAILURE);
    }

    // Esperar hilos a finalizar
    pthread_join(thread_dispatch, NULL);
    pthread_join(thread_interrupt, NULL);
    
    // sem_destroy(&mutex_pcb);
    sem_destroy(&mutex_interrupt);
    
    log_destroy(logger);
    config_destroy(config);

    return EXIT_SUCCESS;
}

void* dispatch(void* arg) {
    int dispatch_fd = iniciar_servidor(config_get_string_value(config, KEY_PUERTO_ESCUCHA_DISPATCH));
    log_info(logger, "Dispatch ready - SOCKET: %d", dispatch_fd);

    char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);
    int memoria_fd = crear_conexion(ip_memoria, puerto_memoria);

    log_info(logger, "Connected to Memoria - SOCKET: %d", memoria_fd);

    while(1) {
        int kernel_fd = esperar_cliente(dispatch_fd);
        log_info(logger, "Dispatch client - SOCKET: %d", kernel_fd);

        for (op_code cod_op = recibir_operacion(kernel_fd); cod_op != -1; cod_op = recibir_operacion(kernel_fd)){
            if (cod_op == PCB) {
                pcb = recibir_pcb(kernel_fd, logger);

                log_info(logger, "Se recibio PID: %u", pcb->pid);

                sem_wait(&mutex_interrupt);
                has_interrupt = false;
                sem_post(&mutex_interrupt);

                cpu(memoria_fd, kernel_fd);
            }
        }

        log_error(logger, "Dispatch client disconnected - SOCKET: %d", kernel_fd);
    }
    
    liberar_conexion(memoria_fd);
    close(dispatch_fd);
}

void* interrupt(void* arg) {
    int dispatch_fd = iniciar_servidor(config_get_string_value(config, KEY_PUERTO_ESCUCHA_INTERRUPT));
    log_info(logger, "Interrupt ready - SOCKET: %d", dispatch_fd);

    while(1) {
        int kernel_fd = esperar_cliente(dispatch_fd);
        log_info(logger, "Interrupt client - SOCKET: %d", kernel_fd);

        for (op_code cod_op = recibir_operacion(kernel_fd); cod_op != -1; cod_op = recibir_operacion(kernel_fd)){
            if (cod_op == INTERRUPT) {
                uint32_t pid;
                recv(kernel_fd, &pid, sizeof(uint32_t), MSG_WAITALL);

                log_info(logger, "Interrupcion - PID: %u", pid);

                if (pcb->pid == pid){
                    sem_wait(&mutex_interrupt);
                    has_interrupt = true;
                    sem_post(&mutex_interrupt);
                }
            }
        }

        log_error(logger, "Interrupt client disconnected - SOCKET: %d", kernel_fd);
    }

    close(dispatch_fd);
}