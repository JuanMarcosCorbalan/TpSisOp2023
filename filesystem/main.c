#include "include/main.h"

int main(void) {
	logger = log_create("log.log", "FILESYSTEM", 1, LOG_LEVEL_DEBUG);

	int conexion;
	t_config* config;
	char* ip;
	char* puerto;
	char* puertoCpu;

	config = iniciar_config();
	puerto = config_get_string_value(config, "PUERTO");
	ip = config_get_string_value(config, "IP");
	puertoCpu = config_get_string_value(config, "PUERTOCPU");
	conexion = crear_conexion(ip, puertoCpu);

	int server_fd = iniciar_servidor(puerto);
	log_info(logger, "FILESYSTEM LISTO...");
	int cliente_fd = esperar_cliente(server_fd);

	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(cliente_fd);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(cliente_fd);
			break;
		case PAQUETE:
			lista = recibir_paquete(cliente_fd);
			log_info(logger, "Me llegaron los siguientes valores:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			log_error(logger, "el cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
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

void iterator(char* value) {
	log_info(logger,"%s", value);
}
