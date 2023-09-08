#include "include/main.h"

int main(void) {
	logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_DEBUG);

	t_config* config;
	char* ip;
	char* puerto;
	char* puerto_memoria;
	char* valor;
	int fd_memoria = 0;
	config = iniciar_config();
	ip = config_get_string_value(config, "IP");
	puerto = config_get_string_value(config, "PUERTO");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	valor = config_get_string_value(config, "VALOR");

	fd_memoria = crear_conexion(logger, ip, puerto_memoria);
	enviar_mensaje(valor, fd_memoria);
//	liberar_conexion(fd_memoria);

	int server_fd = iniciar_servidor(puerto);
	log_info(logger, "CPU LISTO...");
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
			log_error(logger, "El cliente se desconecto.");
			cliente_fd = esperar_cliente(server_fd);
			break;
		default:
			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return EXIT_SUCCESS;
}

t_config* iniciar_config(void)
{
	t_config* nuevo_config = config_create("./cpu.config");

	if(nuevo_config == NULL){
		printf("No se pudo leer el config\n");
		exit(2);
	}

	return nuevo_config;
}
