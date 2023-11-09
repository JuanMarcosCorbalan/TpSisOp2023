#include "../include/conexion.h"

t_list* proceso_instrucciones;

static void procesar_cliente(void* void_args){
	t_procesar_cliente_args* args = (t_procesar_cliente_args*) void_args;
	int cliente_fd = args -> fd_cliente;
	t_log* logger = args ->logger;
	free(args);

	t_list* lista;
	while(cliente_fd != -1){
		int cod_op = recibir_operacion(cliente_fd);

		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(logger, cliente_fd);
				break;
			case HANDSHAKE_CPU_MEMORIA:
				send_herramientas_traduccion(cliente_fd, tam_pagina, espacio_usuario);
				break;
			case PAQUETE:
				lista = recibir_paquete(cliente_fd);
				log_info(logger, "Me llegaron los siguientes valores:\n");
				list_iterate(lista, (void*) iterator);
				break;
//			case PAQUETE:
//				lista = recibir_paquete(cliente_fd);
//				log_info(logger, "Me llegaron los siguientes valores:\n");
//				list_iterate(lista, (void*) iterator);
//				break;
			case DATOS_PROCESO_NEW:
				t_datos_proceso* datos_proceso = recv_datos_proceso(cliente_fd);
				iniciar_proceso_memoria(datos_proceso->path, datos_proceso->size, datos_proceso->pid, cliente_fd);
				break;
			case SOLICITAR_INSTRUCCION:
				t_proceso_instrucciones* pruebita = list_get(proceso_instrucciones, 0);
				t_instruccion* pruebita2 = list_get(pruebita->instrucciones, 0);
				procesar_pedido_instruccion(cliente_fd, proceso_instrucciones);
				break;
			case LEER_MEMORIA:
				//recibir direccion
				//dato = leer_espacio_usuario(direccion)
				//mandar dato
				break;
			case ESCRIBIR_MEMORIA:
				//recibir direccion y valor
				//escribir_espacio_usuario(direccion, valor)
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
	proceso_instrucciones = list_create();
	int cliente_socket = esperar_cliente(logger, server_socket);

	if(cliente_socket != -1){
		pthread_t hilo;
		t_procesar_cliente_args* args = malloc(sizeof(t_procesar_cliente_args));
		args -> fd_cliente = cliente_socket;
		args ->logger = logger;
		pthread_create(&hilo, NULL, (void*) procesar_cliente, (void*) args);
		pthread_detach(hilo);
		return 1;
	}

	return 0;
}

void iniciar_proceso_memoria(char* path, int size, int pid, int socket_kernel){
	//INSTRUCCIONES
	t_proceso_instrucciones* proceso_instr = malloc(sizeof(t_proceso_instrucciones));
	char* prefijoRutaProceso = "/home/utnso/tp-2023-2c-Sisop-Five/mappa-pruebas/";
	char* extensionProceso = ".txt";
	char* rutaCompleta = string_new();

	string_append(&rutaCompleta, prefijoRutaProceso);
	string_append(&rutaCompleta, path);
	string_append(&rutaCompleta, extensionProceso);

//	t_proceso_instrucciones* proceso_instr = malloc(sizeof(t_proceso_instrucciones));
    t_proceso_instrucciones* proceso_instr = malloc(sizeof(t_proceso_instrucciones));
	proceso_instr->instrucciones = list_create();

	proceso_instr->pid = pid;
	proceso_instr->instrucciones = generar_instrucciones(rutaCompleta);

	list_add(proceso_instrucciones, proceso_instr);
	free(proceso_instr);
	//TABLA DE PAGINAS

	t_tdp* tdp = malloc(sizeof(t_tdp));
	t_list* paginas = list_create();

	int cant_paginas = size / tam_pagina;

	for(int i = 1; i < cant_paginas; i++){
		t_pagina* pag = malloc(sizeof(t_pagina));
		pag->bit_presencia = 0;
		pag->bit_modificado = 0;
		pag->marco = malloc(sizeof(int));
		pag->pos_swap = malloc(sizeof(int)); //TODO preguntarle a fs
		list_add(paginas, pag);
		free(pag);
	}
	tdp->pid = pid;
	tdp->paginas = paginas;

	send_tdp(socket_kernel, tdp);
	log_info(logger, "Tabla de paginas creada. PID: %d - Tamaño: %d\n", pid, cant_paginas); //log obligatorio
	free(tdp);
	list_destroy(paginas);
	t_proceso_instrucciones* pruebita = list_get(proceso_instrucciones, 0);
	t_instruccion* pruebita2 = list_get(pruebita->instrucciones, 0);
//	free(proceso_instr);
}

void procesar_pedido_instruccion(int socket_cpu, t_list* proceso_instrucciones){
	t_solicitud_instruccion* solicitud_instruccion = recv_solicitar_instruccion(socket_cpu);

	t_proceso_instrucciones* pruebita = list_get(proceso_instrucciones, 0);
	t_instruccion* pruebita2 = list_get(pruebita->instrucciones, 0);

	t_instruccion* instruccion_a_enviar = buscar_instruccion(solicitud_instruccion->pid, solicitud_instruccion->program_counter - 1, proceso_instrucciones);
	sleep(retardo_respuesta);
	send_proxima_instruccion(socket_cpu, instruccion_a_enviar);
}

t_instruccion* buscar_instruccion(int pid, int program_counter, t_list* proceso_instrucciones){
	int i = 0;
	t_proceso_instrucciones* proceso_instr = list_get(proceso_instrucciones, i);

	while(pid != proceso_instr->pid){
		i++;
		proceso_instr = list_get(proceso_instrucciones, i);
	}

	return list_get(proceso_instr->instrucciones, program_counter);
}

t_list* generar_instrucciones(char* path) {
	t_list* instrucciones = list_create();
	char linea[MAX_LINE_LENGTH];
	char ** token_instruccion;
	int cant_parametros;
	FILE* archivo = fopen(path, "r");

	while(fgets(linea, MAX_LINE_LENGTH, archivo)){
		t_instruccion* instruccion = malloc(sizeof(t_instruccion));
		//Eliminar el ultimo \n
		size_t longitud = strcspn(linea, "\n");
		linea[longitud] = '\0';

		token_instruccion = string_split(linea, " ");
		for(int i=0; token_instruccion[i] != NULL; i++){
			cant_parametros=i;
		}

		instruccion->codigo = instruccion_to_enum(token_instruccion[0]);
		if(cant_parametros == 0){
			instruccion->param1 = "";
			instruccion->param2 = "";
		} else if(cant_parametros == 1){
			instruccion->param1 = token_instruccion[1];
			instruccion->param2 = "";
		} else if(cant_parametros == 2){
			instruccion->param1 = token_instruccion[1];
			instruccion->param2 = token_instruccion[2];;
		}
		list_add(instrucciones, instruccion);
		t_instruccion* pruebita = list_get(instrucciones, 0);
		//TODO Faltan los free??
	}
	return instrucciones;
}

codigo_instruccion instruccion_to_enum(char* instruccion){
	if(strcmp(instruccion, "SET") == 0){
		return SET;
	} else if(strcmp(instruccion, "SUM") == 0){
		return SUM;
	} else if(strcmp(instruccion, "SUB") == 0){
		return SUB;
	}  else if(strcmp(instruccion, "EXIT") == 0){
		return EXIT;
	}

	return EXIT_FAILURE;
}

uint32_t leer_espacio_usuario(uint32_t direccion) {
	uint32_t valor;

	//mutex
	memcpy(&valor, espacio_usuario + direccion, sizeof(uint32_t));

	return valor;
}

void escribir_espacio_usuario(uint32_t direccion, uint32_t valor) {
	//mutex
	memcpy(espacio_usuario + direccion, &valor, sizeof(int));

}
