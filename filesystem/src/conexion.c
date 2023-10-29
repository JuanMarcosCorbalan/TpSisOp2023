#include "../include/conexion.h"

typedef struct {
    int fd_cliente;
    t_log* logger;
} t_procesar_cliente_args;

static void procesar_cliente(void* void_args){
	t_procesar_cliente_args* args = (t_procesar_cliente_args*) void_args;
	int cliente_fd = args -> fd_cliente;
	t_log* logger = args ->logger;
	free(args);

	while(cliente_fd != -1){
		int cod_op = recibir_operacion(cliente_fd);

		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(cliente_fd);
				break;
			case -1:
				log_error(logger, "El cliente se desconecto.");
				return;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				return;
			}
	}

	return;
}

int experar_clientes(t_log* logger, int server_socket){

	int cliente_socket = esperar_cliente(logger, server_socket);

	if(cliente_socket != -1){
		pthread_t hilo;
		t_procesar_cliente_args* args = malloc(sizeof(t_procesar_cliente_args));
		args -> fd_cliente = cliente_socket;
		args -> logger = logger;
		pthread_create(&hilo, NULL, (void*) procesar_cliente, (void*) args);
		pthread_detach(hilo);
		return 1;
	}

	return 0;
}
