#include "main.h"

void* interrupt_main(void *arg){
    log_info(logger, "Thread interrupt_main creado");
    char* ip_kernel = config_get_string_value(config, KEY_IP_CPU);
    char* puerto_kernel = config_get_string_value(config, KEY_PUERTO_CPU_INTERRUPT);

    int conexion = crear_conexion(ip_kernel, puerto_kernel);
    log_info(logger, "Connected to CPU Interrupt -  SOCKET: %d", conexion);

    while (1){
        t_paquete* paquete;

        sem_wait(&hay_interrupt);
        sem_wait(&mutex_interrupt);
        paquete = crear_paquete(interrupt_pid->type_interrupt);
        agregar_uint_a_paquete(paquete, &interrupt_pid->pid, sizeof(uint32_t));
        sem_post(&mutex_interrupt);
        
        enviar_paquete(paquete, conexion);
        eliminar_paquete(paquete);

        log_info(logger, "Enviada Interrupcion PID: %u", interrupt_pid->pid);
    }

	liberar_conexion(conexion);

    return EXIT_SUCCESS;
}

pthread_t create_quantum_thread(t_quantum* pid_quantum){
    pthread_t q_thread;

    if (pthread_create(&q_thread, NULL, quantum_thread, pid_quantum)) {
        log_error(logger, "Problema al crear hilo para el manejo de Interrupt por Quantum");
        exit(EXIT_FAILURE);
    } else pthread_detach(q_thread);
    

    return q_thread;
}

void* quantum_thread(void* q){
    t_quantum* pid_quantum = (t_quantum*) q;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    log_info(logger, "Comienzo Quantum - PID: %u - Tiempo : %u", pid_quantum->pid, pid_quantum->quantum);
    usleep(pid_quantum->quantum * 1000);

    pthread_cleanup_push(cleanup_thread, pid_quantum);
    pthread_testcancel();
    pthread_cleanup_pop(0);

    sem_wait(&mutex_interrupt);
    interrupt_pid->pid = pid_quantum->pid;
    interrupt_pid->type_interrupt = INTERRUPT;
    sem_post(&mutex_interrupt);
    sem_post(&hay_interrupt);

    free(pid_quantum);

    return EXIT_SUCCESS;

}

void cleanup_thread(void *arg) {
    free(arg);
}