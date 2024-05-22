#ifndef UTILS_CLIENT_H_
#define UTILS_CLIENT_H_

#include<utils/general.h>

int crear_conexion(char*, char*);
void enviar_mensaje(char*, int);
t_paquete* crear_paquete(op_code);
void agregar_a_paquete(t_paquete*, void*, int);
void agregar_uint_a_paquete(t_paquete*, void*, int); // pasar a utils
void enviar_paquete(t_paquete*, int);
void liberar_conexion(int);
void eliminar_paquete(t_paquete*);


#endif