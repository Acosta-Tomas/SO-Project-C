#include "mem_protocol.h"

t_init_pid* recibir_init_process(int conexion, t_log* logger){
    int size;
    int desplazamiento = 0;
    void* buffer;
    t_init_pid* pid_to_init = malloc(sizeof(t_init_pid));
	int string_size;

    buffer = recibir_buffer(&size, conexion);

    memcpy(&pid_to_init->pid, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&string_size, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
	pid_to_init->path = malloc(string_size);
    memcpy(pid_to_init->path, buffer + desplazamiento, string_size);
    desplazamiento += string_size;

    if (size != desplazamiento) log_info(logger, "Error al recibir PID Para iniciar");

	free(buffer);

    return pid_to_init;
}

void agregar_init_process_paquete(t_paquete* paquete, uint32_t pid, char* path){
	agregar_uint_a_paquete(paquete, &pid, sizeof(uint32_t));
	agregar_a_paquete(paquete, path, strlen(path) + 1);
}
