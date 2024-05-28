#include "io_protocol.h"

t_io* recibir_io(int conexion, t_log* logger){
	int size;
    int desplazamiento = 0;
    void* buffer;
    t_io* io = malloc(sizeof(t_io));
	int string_size;

    buffer = recibir_buffer(&size, conexion);

    memcpy(&io->type_instruction, buffer + desplazamiento, sizeof(set_instruction));
    desplazamiento += sizeof(uint32_t);

    memcpy(&string_size, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
	io->name_interface = malloc(string_size);
    memcpy(io->name_interface, buffer + desplazamiento, string_size);
    desplazamiento += string_size;

	memcpy(&string_size, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
	io->sleep_time = malloc(string_size);
	memcpy(io->sleep_time, buffer + desplazamiento, string_size);
    desplazamiento += string_size;

    if (size != desplazamiento) log_info(logger, "Error al recibir PID Para iniciar");

	free(buffer);

    return io;
}

void agregar_io_paquete(t_paquete* paquete, set_instruction instruction, char* interfaz, char* time){
	agregar_uint_a_paquete(paquete, &instruction, sizeof(set_instruction));
	agregar_a_paquete(paquete, interfaz, strlen(interfaz) + 1);
	agregar_a_paquete(paquete, time, strlen(time) + 1);
}