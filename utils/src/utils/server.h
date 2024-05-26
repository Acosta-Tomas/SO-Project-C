#ifndef UTILS_SERVER_H_
#define UTILS_SERVER_H_

#include<utils/general.h>

void* recibir_buffer(int*, int);
int iniciar_servidor(char*);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int, t_log*);
int recibir_operacion(int);
void agregar_pcb_paquete(t_paquete*, t_pcb*);
t_init_pid* recibir_init_process(int, t_log*);
t_io* recibir_io(int, t_log*);

#endif