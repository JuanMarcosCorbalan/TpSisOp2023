#include "../include/conexion.h"

typedef struct {
    int fd_cliente;
    t_log* logger;
} t_procesar_cliente_args;

static void procesar_cliente(void* void_args){
	t_procesar_cliente_args* args = (t_procesar_cliente_args*) void_args;
	int cliente_fd = args -> fd_cliente;
	t_log* logger = args->logger;
	free(args);

//	datos_procesos = list_create();
//	t_list* lista;
	while(cliente_fd != -1){
		int cod_op = recibir_operacion(cliente_fd);

		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(logger, cliente_fd);
				break;
//			case PAQUETE:
//				lista = recibir_paquete(cliente_fd);
//				log_info(logger, "Me llegaron los siguientes valores:\n");
//				list_iterate(lista, (void*) iterator);
//				break;
			case DATOS_PROCESO_NEW:
				t_datos_proceso* datos = recv_datos_proceso(cliente_fd);
				log_info(logger, "Recibi el nuevo proceso %d ubicado en %s", datos->pid, datos->path);
//				list_add(datos_procesos, datos);
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

int esperar_clientes(t_log* logger, int server_socket){

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
