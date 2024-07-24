#include "main.h"

t_log* logger;
t_config* config;
t_pcb* pcb;
uint32_t page_size;

pthread_t thread_dispatch, thread_interrupt;

bool has_interrupt = false;
op_code interrupt_type;

sem_t mutex_interrupt;

bool isLRU;
int numero_entradas;
t_queue* tlb_queue;

int main(int argc, char* argv[]) {
    config = argc > 1 ? config_create(argv[1]) : config_create(CONFIG_FILE);

    if (config == NULL) exit(EXIT_FAILURE); 

    logger = log_create(LOGS_FILE, config_get_string_value(config, KEY_SERVER_LOG), true, log_level_from_string(config_get_string_value(config, KEY_LOG_LEVEL)));
    numero_entradas = config_get_int_value(config, KEY_CANTIDAD_ENTRADAS_TLB);
    isLRU = !(strcmp(config_get_string_value(config, KEY_ALGORITMO_TLB), "LRU"));
    tlb_queue = queue_create();

    sem_init(&mutex_interrupt, 0, 1);

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
    
    sem_destroy(&mutex_interrupt);
    
    queue_destroy(tlb_queue); //Tengo que iterar para eliminar lo de adentro.
    log_destroy(logger);
    config_destroy(config);

    return EXIT_SUCCESS;
}

void* dispatch(void* arg) {
    int dispatch_fd = iniciar_servidor(config_get_string_value(config, KEY_PUERTO_ESCUCHA_DISPATCH));
    log_info(logger, "SOCKET: %d - Dispatch", dispatch_fd);

    char* ip_memoria = config_get_string_value(config, KEY_IP_MEMORIA);
    char* puerto_memoria = config_get_string_value(config, KEY_PUERTO_MEMORIA);
    int memoria_fd = crear_conexion(ip_memoria, puerto_memoria);
    op_code code_op = MEM_PAGE_SIZE;

    send(memoria_fd, &code_op , sizeof(op_code), 0);
    recv(memoria_fd, &page_size, sizeof(uint32_t), MSG_WAITALL);

    log_info(logger, "SOCKET: %d - Memoria - Tamaño pagina: %u", memoria_fd, page_size);

    while(1) {
        int kernel_fd = esperar_cliente(dispatch_fd);
        log_info(logger, "SOCKET: %d - Dispatch - Kernel conectado", kernel_fd);

        for (op_code cod_op = recibir_operacion(kernel_fd); cod_op != -1; cod_op = recibir_operacion(kernel_fd)){
            if (cod_op == PCB) {
                pcb = recibir_pcb(kernel_fd, logger);

                log_debug(logger, "Se recibio PID: %u", pcb->pid);

                sem_wait(&mutex_interrupt);
                has_interrupt = false;
                sem_post(&mutex_interrupt);

                cpu(memoria_fd, kernel_fd);
            }
        }

        log_error(logger, "SOCKET: %d - Dispatch - Kernel desconectado", kernel_fd);
    }
    
    liberar_conexion(memoria_fd);
    close(dispatch_fd);
}

void* interrupt(void* arg) {
    int dispatch_fd = iniciar_servidor(config_get_string_value(config, KEY_PUERTO_ESCUCHA_INTERRUPT));
    log_info(logger, "SOCKET: %d - Interrupt ", dispatch_fd);

    while(1) {
        int kernel_fd = esperar_cliente(dispatch_fd);
        log_info(logger, "SOCKET: %d - Interrupt - Kernel conectado", kernel_fd);

        for (op_code cod_op = recibir_operacion(kernel_fd); cod_op != -1; cod_op = recibir_operacion(kernel_fd)){
            if (cod_op == INTERRUPT || cod_op == END_PID_USER) {
                uint32_t pid, size_discard;

                recv(kernel_fd, &size_discard, sizeof(u_int32_t), MSG_WAITALL); // Recibo tamaño buffer pero no me interesa (se manda por la serializacion de codigo de operacion con uint32)
                recv(kernel_fd, &pid, sizeof(uint32_t), MSG_WAITALL);

                log_debug(logger, "Interrupcion - PID: %u", pid);

                if (pcb->pid == pid){
                    sem_wait(&mutex_interrupt);
                    has_interrupt = true;
                    interrupt_type = cod_op;
                    sem_post(&mutex_interrupt);
                }
            }
        }

        log_error(logger, "SOCKET: %d - Interrupt - Kernel desconectado", kernel_fd);
    }

    close(dispatch_fd);
}