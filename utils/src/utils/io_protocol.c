#include "io_protocol.h"

t_io* recibir_io(int conexion, char** name_interface, t_log* logger){
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
	*(name_interface) = (char*)malloc(string_size);

    memcpy(*(name_interface), buffer + desplazamiento, string_size);
    desplazamiento += string_size;

	memcpy(&io->buffer_size, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
	io->buffer = malloc(io->buffer_size);

	memcpy(io->buffer, buffer + desplazamiento, io->buffer_size);
    desplazamiento += io->buffer_size;

    if (size != desplazamiento) log_info(logger, "Error al recibir IO de CPU");

	free(buffer);

    return io;
}

void agregar_io_paquete(t_paquete* paquete, set_instruction instruction, char* params[], int total_params){
	agregar_uint_a_paquete(paquete, &instruction, sizeof(set_instruction));

    for(int i = 0; i < total_params; i += 1){
        agregar_a_paquete(paquete, params[i], strlen(params[i]) + 1);
    }
}

void agregar_io_serializado(t_paquete* paquete, t_io* io){
    agregar_uint_a_paquete(paquete, &io->type_instruction, sizeof(set_instruction));
    agregar_a_paquete(paquete, io->buffer, io->buffer_size);
}

t_io* recibir_io_serializado(int conexion, t_log* logger){
	int size;
    int desplazamiento = 0;
    void* buffer;
    t_io* io = malloc(sizeof(t_io));

    buffer = recibir_buffer(&size, conexion);

    memcpy(&io->type_instruction, buffer + desplazamiento, sizeof(set_instruction));
    desplazamiento += sizeof(uint32_t);

	memcpy(&io->buffer_size, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
	io->buffer = malloc(io->buffer_size);

	memcpy(io->buffer, buffer + desplazamiento, io->buffer_size);
    desplazamiento += io->buffer_size;

    if (size != desplazamiento) log_info(logger, "Error al recibir IO de CPU");

	free(buffer);

    return io;
}