#include "io_protocol.h"

t_io* recibir_io(int conexion, char** name_interface, t_log* logger){
	int size;
    int desplazamiento = 0;
    void* buffer;
    t_io* io = malloc(sizeof(t_io));
	int string_size;

    buffer = recibir_buffer(&size, conexion);

    memcpy(&io->type_instruction, buffer + desplazamiento, sizeof(set_instruction));
    desplazamiento += sizeof(set_instruction);

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

    if (size != desplazamiento) log_error(logger, "Error al recibir IO");

	free(buffer);

    return io;
}

void agregar_io_paquete(t_paquete* paquete, set_instruction instruction, char* params[], int total_params){
	agregar_uint_a_paquete(paquete, &instruction, sizeof(set_instruction));

    for(int i = 0; i < total_params; i += 1){
        agregar_a_paquete(paquete, params[i], strlen(params[i]) + 1);
    }
}

void agregar_io_serializado(t_paquete* paquete, t_io* io, uint32_t pid){
    agregar_uint_a_paquete(paquete, &pid, sizeof(uint32_t));
    agregar_uint_a_paquete(paquete, &io->type_instruction, sizeof(set_instruction));
    agregar_a_paquete(paquete, io->buffer, io->buffer_size);
}

t_io* recibir_io_serializado(int conexion, uint32_t* pid, t_log* logger){
	int size;
    int desplazamiento = 0;
    void* buffer;
    t_io* io = malloc(sizeof(t_io));

    buffer = recibir_buffer(&size, conexion);

    memcpy(pid, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(&io->type_instruction, buffer + desplazamiento, sizeof(set_instruction));
    desplazamiento += sizeof(set_instruction);

	memcpy(&io->buffer_size, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
	io->buffer = malloc(io->buffer_size);

	memcpy(io->buffer, buffer + desplazamiento, io->buffer_size);
    desplazamiento += io->buffer_size;

    if (size != desplazamiento) log_error(logger, "Error al recibir IO");

	free(buffer);

    return io;
}

uint32_t get_io_frames(t_io* io, t_list* frames){
    uint32_t desplazamiento = 0;
    uint32_t size_io;

    memcpy(&size_io, io->buffer, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    while(desplazamiento < io->buffer_size){
        t_memoria_fisica* frame = malloc(sizeof(t_memoria_fisica));

        memcpy(&frame->direccion_fisica, io->buffer + desplazamiento, sizeof(uint32_t));
        desplazamiento += sizeof(uint32_t);

        memcpy(&frame->bytes, io->buffer + desplazamiento, sizeof(uint32_t));
        desplazamiento += sizeof(uint32_t);

        list_add(frames, frame);
    }

    return size_io;
}

char* get_io_file(t_io* io, char* path){
    char* file_name = malloc(io->buffer_size); 
    char* fiel_path = string_duplicate(path);

    memcpy(file_name, io->buffer, io->buffer_size);
    string_append(&fiel_path, file_name);

    free(file_name);
    free(io->buffer);
    free(io);
    return fiel_path;
}

char* get_io_truncate(t_io* io, char* path, int* size){
    uint32_t file_size;
    uint32_t desplazamiento = 0;
    char* fiel_path = string_duplicate(path);

    memcpy(&file_size, io->buffer, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    char* file_name = malloc(file_size); 

    memcpy(file_name, io->buffer + desplazamiento, file_size);
    desplazamiento += file_size;
    
    memcpy(size, io->buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    if(io->buffer_size != desplazamiento) {
        free(file_name);
        free(io->buffer);
        free(io);
        return NULL;
    }

    string_append(&fiel_path, file_name);
    free(io->buffer);
    free(io);
    return fiel_path;
}

char* get_io_read_write(t_io* io, char* path, t_fs_rw* fs_info) {
    fs_info->frames = list_create();
    uint32_t desplazamiento = 0;
    char* fiel_path = string_duplicate(path);
    uint32_t file_size;

    memcpy(&file_size, io->buffer, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    char* file_name = malloc(file_size); 

    memcpy(file_name, io->buffer + desplazamiento, file_size);
    desplazamiento += file_size;

    memcpy(&fs_info->ptr, io->buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(&fs_info->size, io->buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    while(desplazamiento < io->buffer_size){
        t_memoria_fisica* frame = malloc(sizeof(t_memoria_fisica));

        memcpy(&frame->direccion_fisica, io->buffer + desplazamiento, sizeof(uint32_t));
        desplazamiento += sizeof(uint32_t);

        memcpy(&frame->bytes, io->buffer + desplazamiento, sizeof(uint32_t));
        desplazamiento += sizeof(uint32_t);

        list_add(fs_info->frames, frame);
    }

    if(io->buffer_size != desplazamiento) {
        list_destroy(fs_info->frames);
        free(fs_info);
        free(file_name);
        free(io->buffer);
        free(io);
        return NULL;
    }

    string_append(&fiel_path, file_name);
    free(io->buffer);
    free(io);
    return fiel_path;
}