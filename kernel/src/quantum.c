#include "main.h"

void* quantum_main(void *arg){
    log_info(logger, "Thread Quatum creado");
    char* ip_kernel = config_get_string_value(config, KEY_IP_CPU);
    char* puerto_kernel = config_get_string_value(config, KEY_PUERTO_CPU_INTERRUPT);

    int conexion = crear_conexion(ip_kernel, puerto_kernel);
    log_info(logger, "Connected to CPU Interrupt -  SOCKET: %d", conexion);

    op_code cod_op = INTERRUPT;

    while (1){
        sem_wait(&start_quantum);
        log_info(logger, "Comienzo Quantum - PID: %u - Tiempo :%u", running_pid->pid, running_pid->quantum);
        usleep(running_pid->quantum * 1000);
        send(conexion, &cod_op, sizeof(op_code), 0);
        send(conexion, &running_pid->pid,  sizeof(uint32_t), 0);
        log_info(logger, "Enviada Interrupcion");
    }

	liberar_conexion(conexion);

    return EXIT_SUCCESS;
}