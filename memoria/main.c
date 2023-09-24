#include "include/main.h"

int main() {
	logger = log_create("memoria.log", "MEMORIA", true, LOG_LEVEL_INFO);

	t_config* config;
	config = iniciar_config();
	char* puerto_escucha;
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	int server_fd = iniciar_servidor(puerto_escucha);
	log_info(logger, "MEMORIA LISTO...");
    while (experar_clientes(server_fd));

    return 0;
}

t_config* iniciar_config(void)
{
	t_config* nuevo_config = config_create("./memoria.config");

	if(nuevo_config == NULL){
		printf("No se pudo leer el config\n");
		exit(2);
	}

	return nuevo_config;
}
