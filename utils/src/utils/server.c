#include <utils/server.h>

int iniciar_servidor(char* puerto){
	int socket_servidor;
    int err;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	err = getaddrinfo(NULL, puerto, &hints, &servinfo);

    if (err != 0) exit(EXIT_FAILURE);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo->ai_family, 
								servinfo->ai_socktype, 
								servinfo->ai_protocol);

	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);
	// Escuchamos las conexiones entrantes
	listen(socket_servidor, 2);

	freeaddrinfo(servinfo);

	return socket_servidor;
}

int esperar_cliente(int socket_servidor){
	int socket_cliente = accept(socket_servidor, NULL, NULL);
	return socket_cliente;
}

int recibir_operacion(int socket_cliente){
	int cod_op;

	// Recibir codigo operacion
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0) return cod_op;
	else{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente){
	void * buffer;

	// Recibo el tamaño de lo que vamos a leer
	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	// Guardo espacio para ese tamaño y espero que lo manden
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente, t_log* logger){
	int size;

	char* buffer = recibir_buffer(&size, socket_cliente); // Sabemos que es char pq es un msj string
	
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

t_list* recibir_paquete(int socket_cliente){
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	// Recibimos el bufffer entero serialiado
	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int)); // Copiamos el size del dato (es un int siempre)
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio); // reservamos memoria para ese dato que viene
		memcpy(valor, buffer+desplazamiento, tamanio); // guardamos el valor 
		desplazamiento+=tamanio;
		list_add(valores, valor);	// Agregamos a la lista
	}
	free(buffer);
	return valores;
}


t_pcb* recibir_pcb(int conexion, t_log* logger){
    int size;
    int desplazamiento = 0;
    void* buffer;
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->registers = malloc(sizeof(t_registros));

    buffer = recibir_buffer(&size, conexion);

    memcpy(&pcb->pid, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->pc, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->quantum, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->status, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->ax, buffer + desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&pcb->registers->bx, buffer + desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&pcb->registers->cx, buffer + desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&pcb->registers->dx, buffer + desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&pcb->registers->eax, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->ebx, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->ecx, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->edx, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->si, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->di, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&pcb->registers->pc, buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    if (size != desplazamiento) log_info(logger, "Error al recibir PCB");

	free(buffer);

    return pcb;
}

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