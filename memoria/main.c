#include "include/main.h"

t_log* logger;

int main() {
	logger = log_create("memoria.log", "MEMORIA", true, LOG_LEVEL_INFO);
	config = iniciar_config();

	retardo_respuesta = atoi(config_get_string_value(config, "RETARDO_RESPUESTA"));
	tam_memoria = atoi(config_get_string_value(config, "TAM_MEMORIA"));
	tam_pagina = atoi(config_get_string_value(config, "TAM_PAGINA"));
	cant_marcos = tam_memoria/tam_pagina;
	espacio_usuario = malloc(tam_memoria);
	bitmap_marcos = inicializar_bitmap_marcos();
	char* puerto_escucha;
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	int server_fd = iniciar_servidor(puerto_escucha);
	log_info(logger, "MEMORIA LISTO...");
    while (experar_clientes(logger, server_fd));

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

void liberar_proceso(int server_fd)
{
// 	t_datos_proceso* proceso_a_finalizar;
	recv_datos_proceso(server_fd);

}

char* inicializar_bitmap_marcos(void){
	char* bitmap = malloc(cant_marcos);

	for(int i = 0; i < cant_marcos-1; i++){
		bitmap[i] = '0';
	}
	return bitmap;
}


