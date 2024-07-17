#include "main.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Uso: %s <nombre_interfaz> <config_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    t_config* config = config_create(argv[2]);
    char* nombre_interfaz = argv[1];

    if (config == NULL) exit(EXIT_FAILURE); 

    char* tipo_interfaz = config_get_string_value(config, KEY_TIPO_INTERFAZ);

    switch (mapInterfaz(tipo_interfaz)){
        case GENERICA: generica_io(nombre_interfaz, config); break;

        case STDIN: stdin_io(nombre_interfaz, config); break;

        case STDOUT: stdout_io(nombre_interfaz, config); break;

        case DIALFS: dialfs_io(nombre_interfaz, config); break;
    
        default: generica_io(nombre_interfaz, config); break;
    }

	config_destroy(config);

    return EXIT_SUCCESS;
}

t_interfaz mapInterfaz (char* interfaz) {
    if (strcmp(interfaz, "STDIN") == 0) return STDIN;
    if (strcmp(interfaz, "STDOUT") == 0) return STDOUT;
    if (strcmp(interfaz, "GENERICA") == 0) return GENERICA;
    if (strcmp(interfaz, "DIALFS") == 0) return DIALFS;
    return GENERICA;
}