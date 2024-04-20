#include <utils/general.h>

void decir_hola(char* quien) {
    printf("Hola desde %s!!\n", quien);
}

void leer_consola(t_log* logger){
	char* leido;

	for (leido = readline("> "); leido && strcmp(leido, ""); leido = readline("> ")){
		log_info(logger, "%s", leido);
		free(leido);
	}
	
	if (leido != NULL) free(leido);

	return;
}