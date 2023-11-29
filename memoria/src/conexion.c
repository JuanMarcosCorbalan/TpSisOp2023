#include "../include/conexion.h"

t_list* proceso_instrucciones;
t_list* tablas_de_paginas;
int tam_pagina;
int tam_memoria;
int retardo_respuesta;
int cant_marcos;
int contador_marcos;
char* bitmap_marcos;
void* espacio_usuario;
char* algoritmo_reemplazo;
t_list* paginas_en_memoria = list_create();
int contador_instante = 0;

static void procesar_cliente(void* void_args){
	t_procesar_cliente_args* args = (t_procesar_cliente_args*) void_args;
	int cliente_fd = args -> fd_cliente;
	t_log* logger = args ->logger;
	t_config* config =  args->config;
	free(args);

	tam_pagina = atoi(config_get_string_value(config, "TAM_PAGINA"));
	tam_memoria = atoi(config_get_string_value(config, "TAM_MEMORIA"));
	retardo_respuesta = atoi(config_get_string_value(config, "RETARDO_RESPUESTA"));
	algoritmo_reemplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
	cant_marcos = tam_memoria / tam_pagina;
	bitmap_marcos = inicializar_bitmap_marcos();

	espacio_usuario = malloc(tam_memoria);

	memset(espacio_usuario, 0, tam_memoria);

	tablas_de_paginas = list_create();

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
			case DATOS_PROCESO_NEW:
				t_datos_proceso* datos_proceso = recv_datos_proceso(cliente_fd);
				iniciar_proceso_memoria(datos_proceso->path, datos_proceso->size, datos_proceso->pid, cliente_fd, logger);
				break;
			case SOLICITAR_INSTRUCCION:
				t_proceso_instrucciones* pruebita = list_get(proceso_instrucciones, 0);
				t_instruccion* pruebita2 = list_get(pruebita->instrucciones, 0);
				procesar_pedido_instruccion(cliente_fd, proceso_instrucciones);
				break;
			case MARCO:
				procesar_solicitud_marco(cliente_fd);
				break;
			case CARGAR_PAGINA:
				int* pid;
				int* numero_pagina;
				recv_numero_pagina(pid, numero_pagina, cliente_fd);
				cargar_pagina(*pid, *numero_pagina);
				//mandar respuesta a kernel
				enviar_operacion(PAGINA_CARGADA, cliente_fd);
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

int experar_clientes(t_log* logger, int server_socket, t_config* config){
	proceso_instrucciones = list_create();
	int cliente_socket = esperar_cliente(logger, server_socket);

	if(cliente_socket != -1){
		pthread_t hilo;
		t_procesar_cliente_args* args = malloc(sizeof(t_procesar_cliente_args));
		args -> fd_cliente = cliente_socket;
		args ->logger = logger;
		args->config = config;
		pthread_create(&hilo, NULL, (void*) procesar_cliente, (void*) args);
		pthread_detach(hilo);
		return 1;
	}

	return 0;
}

void iniciar_proceso_memoria(char* path, int size, int pid, int socket_kernel, t_log* logger){
	//INSTRUCCIONES
	t_proceso_instrucciones* proceso_instr = malloc(sizeof(t_proceso_instrucciones));
	char* prefijoRutaProceso = "/home/utnso/tp-2023-2c-Sisop-Five/mappa-pruebas/";
	char* extensionProceso = ".txt";
	char* rutaCompleta = string_new();

	string_append(&rutaCompleta, prefijoRutaProceso);
	string_append(&rutaCompleta, path);
	string_append(&rutaCompleta, extensionProceso);

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
		pag->marco = -1;
		pag->bit_presencia = 0;
		pag->bit_modificado = 0;
		// pag->pos_swap = ??//TODO preguntarle a fs
		list_add(paginas, pag);
		free(pag);
	}
	tdp->pid = pid;
	tdp->paginas = paginas;

	list_add(tablas_de_paginas, tdp);
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

char* inicializar_bitmap_marcos(void){
	char* bitmap = malloc(cant_marcos);

	for(int i = 0; i < cant_marcos-1; i++){
		bitmap[i] = '0';
	}
	return bitmap;
}

void procesar_solicitud_marco(int fd_cpu){
	int* pid;
	int* numero_pagina;
	recv_solicitud_marco(fd_cpu, pid, numero_pagina);

	//buscar proceso en tdps
	int _encontrar_pid(t_tdp *t) {
		return t->pid == *pid;
	}
	t_tdp* tdp = list_find(tablas_de_paginas, (void*) _encontrar_pid);

	//buscar pagina en tdp
	t_pagina* pagina = list_get(tdp->paginas, *numero_pagina);

	send_marco(fd_cpu, pagina->marco);

}

void cargar_pagina(int pid, int numero_pagina){
	//TODO Decirle a fs que me traiga la pagina

	//ver si la memoria esta llena (bitmap de marcos)
	bool flag_memoria_llena = true;
	int marco;
	for(marco = 0; marco < cant_marcos; marco++){
		if(bitmap_marcos[marco] == '0'){
			flag_memoria_llena = false;
			break;
		}
	}
	//en ese caso, ejecutar algoritmo de reemplazo
	if(flag_memoria_llena){
		//algoritmo de reemplazo
		realizar_reemplazo(pid, numero_pagina);
	}else{
		//sino - cargar pagina (asignarle un marco si no tiene y cambiarle el bit de presencia a 1)

		//TODO delegar a otra funcion "
		//pagina->marco = marco;
		//pagina->bit_presencia = 1;
		//pagina->instante_de_referencia = contador_instante;
		bitmap_marcos[marco] = '1';
		//list_add(paginas_en_memoria, pagina);
	}

}

void realizar_reemplazo(int pid, int numero_pagina){ //TODO
	if(!strcmp(algoritmo_reemplazo, "FIFO")){
		t_pagina* pagina_victima = list_remove(paginas_en_memoria, 0);

	}
	else if(!strcmp(algoritmo_reemplazo, "LRU")){
		void* _paginas_menor_referencia(t_pagina* pagina1, t_pagina* pagina2) {
		    return pagina1->instante_de_referencia <= pagina2->instante_de_referencia ? pagina1 : pagina2;
		}

		t_pagina* pagina_victima = list_get_minimum(paginas_en_memoria, (void*) _paginas_menor_referencia);
	}
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
