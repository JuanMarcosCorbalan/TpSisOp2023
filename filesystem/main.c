#include "include/main.h"

int main(void) {
	logger = log_create("filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_DEBUG);

	t_config* config;
	char* ip;
	char* puerto_escucha;
	char* puerto_memoria;
	int fd_memoria = 0;
	config = iniciar_config();
	ip = config_get_string_value(config, "IP");
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

	fd_memoria = crear_conexion(logger, ip, puerto_memoria);
	enviar_mensaje("Hola, soy un File System!", fd_memoria);

	int server_fd = iniciar_servidor(puerto_escucha);
	log_info(logger, "FILESYSTEM LISTO...");
	while(experar_clientes(logger, server_fd));

	return EXIT_SUCCESS;
}

t_config* iniciar_config(void)
{
	t_config* nuevo_config = config_create("./filesystem.config");

	if(nuevo_config == NULL){
		printf("No se pudo leer el config\n");
		exit(2);
	}

	return nuevo_config;
}
