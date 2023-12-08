#include "include/main.h"

t_log* logger;

int main() {
	logger = log_create("memoria.log", "MEMORIA", true, LOG_LEVEL_INFO);
	t_config* config;
	config = iniciar_config();
	char* puerto_escucha;
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	int server_fd = iniciar_servidor(puerto_escucha);
	log_info(logger, "MODULO MEMORIA LISTO.");
    while (experar_clientes(logger, server_fd));

    return 0;
}

void liberar_proceso(int server_fd)
{
// 	t_datos_proceso* proceso_a_finalizar;
	recv_datos_proceso(server_fd);

}
