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

    if (size != desplazamiento) log_error(logger, "Error al recibir PID Para iniciar");

	free(buffer);

    return pid_to_init;
}

void agregar_init_process_paquete(t_paquete* paquete, uint32_t pid, char* path){
	agregar_uint_a_paquete(paquete, &pid, sizeof(uint32_t));
	agregar_a_paquete(paquete, path, strlen(path) + 1);
}

void* recibir_mem_write(int conexion, uint32_t* direccion_fisica, int* value_size, t_log* logger) {
    int size;
    int desplazamiento = 0;
    void* buffer;

    buffer = recibir_buffer(&size, conexion);

    memcpy(direccion_fisica, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(value_size, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
	void* value = malloc(*(value_size));

    memcpy(value, buffer + desplazamiento, *(value_size));
    desplazamiento += *(value_size);

    if (size != desplazamiento) log_error(logger, "Error en protocolo de escritura en memoria");

	free(buffer);
    return value;
}

void* recibir_mem_read(int conexion, uint32_t* direccion_fisica, int* value_size, t_log* logger){
    int size;
    int desplazamiento = 0;
    void* buffer;

    buffer = recibir_buffer(&size, conexion);

    memcpy(direccion_fisica, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(value_size, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    if (size != desplazamiento) log_error(logger, "Error en protocolo de lectura de memoria");
    free(buffer);
    
    return malloc(*(value_size));
}

uint32_t recibir_pid_con_uint32(int conexion, uint32_t* info, t_log* logger){
    int buffer_size;
    int desplazamiento = 0;
    void* buffer;
    uint32_t pid;

    buffer = recibir_buffer(&buffer_size, conexion);

    memcpy(&pid, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(info, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    if (buffer_size != desplazamiento) log_error(logger, "Error en protocolo");
    free(buffer);

    return pid;
}