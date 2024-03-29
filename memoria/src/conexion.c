#include "../include/conexion.h"

t_list* proceso_instrucciones;
t_list* tablas_de_paginas;
int tam_pagina = 0;
int tam_memoria = 0;
int retardo_respuesta = 0;
int cant_marcos = 0;
char* bitmap_marcos;
void* espacio_usuario;
char* algoritmo_reemplazo;
t_list* paginas_en_memoria;
int contador_instante = 0;
int inicializado = 0;
int fd_filesystem = 0;
t_config* config;
t_log* logger;
pthread_mutex_t mutex_memoria;


static void procesar_cliente(void* void_args){
	t_procesar_cliente_args* args = (t_procesar_cliente_args*) void_args;
	int cliente_fd = args -> fd_cliente;
	logger = args ->logger;
	free(args);

	while(cliente_fd != -1){
		int cod_op = recibir_operacion(cliente_fd);

		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(logger, cliente_fd);
				break;
			case HANDSHAKE_CPU_MEMORIA:
				int recibir_cpu = recv_handshake_cpu_memoria(cliente_fd);
				send_tam_pagina(cliente_fd, tam_pagina);
				log_info(logger, "tamanio de pagina %d enviado a cpu socket %d", tam_pagina, cliente_fd);
				break;
			case HANDSHAKE_FS_MEMORIA:
				int recibir_fs = recv_handshake_fs_memoria(cliente_fd);
				fd_filesystem = cliente_fd;
				break;
			case DATOS_PROCESO_NEW:
				t_datos_proceso* datos_proceso = recv_datos_proceso(cliente_fd);
				iniciar_proceso_memoria(datos_proceso->path, datos_proceso->size, datos_proceso->pid, cliente_fd);
				free(datos_proceso->path);
				free(datos_proceso);
				break;
			case SOLICITAR_INSTRUCCION:
				//t_proceso_instrucciones* pruebita = list_get(proceso_instrucciones, 0);
				//t_instruccion* pruebita2 = list_get(pruebita->instrucciones, 0);
				log_info(logger, "Solicitud de instruccion recibida");
				procesar_pedido_instruccion(cliente_fd, proceso_instrucciones);
				break;
			case SOLICITUD_MARCO:
				procesar_solicitud_marco(cliente_fd);
				break;
			case CARGAR_PAGINA:
				pid_numpag_despl* pnd = recv_numero_pagina(cliente_fd);
				cargar_pagina(pnd->pid, pnd->numero_pagina, pnd->desplazamiento);
				free(pnd);
				//mandar respuesta a kernel
				send_pagina_cargada(cliente_fd);
				break;
			case LEER_MEMORIA:
				//recibir direccion
				pid_direccion* direccion_fisica = recv_solicitud_lectura_memoria(cliente_fd);
				uint32_t dato = leer_espacio_usuario(direccion_fisica->direccion, direccion_fisica->pid);
				free(direccion_fisica);
				//mandar dato
				send_valor_leido_memoria(dato, cliente_fd);
				break;
			case ESCRIBIR_MEMORIA:
				//recibir direccion y valor
				direccion_y_valor* dyv = recv_solicitud_escritura_memoria(cliente_fd);
				escribir_espacio_usuario(dyv->direccion, dyv->valor, dyv->pyn->pid, dyv->pyn->numero_pagina);
				free(dyv);
				break;
			case FINALIZAR_PROCESO_MEMORIA:
				t_pcb* pcb = recv_finalizar_proceso_memoria(cliente_fd);
				finalizar_proceso(pcb);
				break;
			case -1:
				log_error(logger, "El cliente se desconecto.");
				return;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				return;
			}
	}
}

int experar_clientes(t_log* logger, int server_socket){
//	config = iniciar_config();
	if(inicializado != 1){
		config = iniciar_config();
		inicializar_variables(logger, config);
	}

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

void inicializar_variables(t_log* logger, t_config* config){
	proceso_instrucciones = list_create();
	tam_pagina = atoi(config_get_string_value(config, "TAM_PAGINA"));
	tam_memoria = atoi(config_get_string_value(config, "TAM_MEMORIA"));
	retardo_respuesta = atoi(config_get_string_value(config, "RETARDO_RESPUESTA"));
	algoritmo_reemplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
	cant_marcos = tam_memoria / tam_pagina;
	bitmap_marcos = inicializar_bitmap_marcos();

	paginas_en_memoria = list_create();

	espacio_usuario = malloc(tam_memoria);

	memset(espacio_usuario, 0, tam_memoria);

	tablas_de_paginas = list_create();
	pthread_mutex_init(&mutex_memoria, NULL);

	char* ip_fs = config_get_string_value(config, "IP_FILESYSTEM");
	char* puerto_fs = config_get_string_value(config, "PUERTO_FILESYSTEM");
//	fd_filesystem = crear_conexion(logger, ip_fs, puerto_fs);

//	log_info(logger, "MEMORIA LISTO...");
	inicializado = 1;
}

uint32_t* recibir_bloques_reservados(){
	uint32_t* valor;
	while (true) {
		int cod_op = recibir_operacion(fd_filesystem);
		switch (cod_op) {
		case INICIARPROCESO:
			valor = recv_lista_bloques_reservados(fd_filesystem);
			break;
		}
		return valor;
	}
}

void iniciar_proceso_memoria(char* path, int size, int pid, int socket_kernel) {
    // Construir la ruta completa del archivo
    char* rutaCompleta = string_from_format("/home/utnso/tp-2023-2c-Sisop-Five/mappa-pruebas/%s.txt", path);

    // Generar instrucciones
    t_list* instrucciones = generar_instrucciones(rutaCompleta);
    free(rutaCompleta);

    // Crear el objeto de proceso_instrucciones
    t_proceso_instrucciones* proceso_instr = malloc(sizeof(t_proceso_instrucciones));
    proceso_instr->pid = pid;
    proceso_instr->instrucciones = instrucciones;

    list_add(proceso_instrucciones, proceso_instr);

    // Crear la tabla de páginas
    t_tdp* tdp = malloc(sizeof(t_tdp));
    tdp->pid = pid;

    int cant_paginas = size / tam_pagina;
    t_list* paginas = list_create();

    for (int i = 0; i < cant_paginas; i++) {
        t_pagina* pag = malloc(sizeof(t_pagina));
        pag->pid = pid;
        pag->numpag = i;
        pag->marco = -1;
        pag->bit_presencia = 0;
        pag->bit_modificado = 0;
        list_add(paginas, pag);
    }

    tdp->paginas = paginas;

    list_add(tablas_de_paginas, tdp);
    log_info(logger, "Tabla de paginas creada. PID: %d - Tamaño: %d\n", pid, cant_paginas);
}

void procesar_pedido_instruccion(int socket_cpu, t_list* proceso_instrucciones){
//	t_config* config = iniciar_config();
	int retardo_respuesta = config_get_long_value(config, "RETARDO_RESPUESTA");
	t_solicitud_instruccion* solicitud_instruccion = recv_solicitar_instruccion(socket_cpu);
	t_proceso_instrucciones* pruebita = list_get(proceso_instrucciones, 0);
//	t_instruccion* pruebita2 = list_get(pruebita->instrucciones, 0);

	t_instruccion* instruccion_a_enviar = buscar_instruccion(solicitud_instruccion->pid, solicitud_instruccion->program_counter - 1, proceso_instrucciones);
	free(solicitud_instruccion);
	usleep(retardo_respuesta*1000);
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
	} else if(strcmp(instruccion, "JNZ") == 0){
		return JNZ;
	} else if(strcmp(instruccion, "EXIT") == 0){
		return EXIT;
	} else if(strcmp(instruccion, "WAIT") == 0){
		return WAIT;
	} else if(strcmp(instruccion, "SLEEP") == 0){
		return SLEEP;
	} else if(strcmp(instruccion, "SIGNAL") == 0){
		return SIGNAL;

	} else if(strcmp(instruccion, "MOV_OUT") == 0){
		return MOV_OUT;
	} else if(strcmp(instruccion, "MOV_IN") == 0){
		return MOV_IN;
	} else if (strcmp(instruccion, "F_OPEN") == 0){
		return F_OPEN;
	} else if (strcmp(instruccion, "F_CLOSE") == 0){
		return F_CLOSE;
	}
	return EXIT_FAILURE;
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

char* inicializar_bitmap_marcos(void){
	char* bitmap = malloc(cant_marcos);

	for(int i = 0; i < cant_marcos; i++){
		bitmap[i] = '0';
	}
	return bitmap;
}

void procesar_solicitud_marco(int fd_cpu){
	pid_y_numpag* valores = recv_solicitud_marco(fd_cpu);

	//buscar pagina en tdp
	t_pagina* pagina = buscar_pagina(valores->pid, valores->numero_pagina);

	send_marco(fd_cpu, pagina->marco);
	free(valores);
}

t_pagina* buscar_pagina(int pid, int numero_pagina){
	//buscar proceso en tdps
	bool _encontrar_pid(void* t) {
		return (((t_tdp*)t)->pid == pid);
	}
	t_tdp* tdp = list_find(tablas_de_paginas, _encontrar_pid);

	//buscar pagina en tdp
	t_pagina* pagina = list_get(tdp->paginas, numero_pagina);
	log_info(logger, "PID: %d - Pagina: %d - Marco: %d", pid, numero_pagina, pagina->marco);
	return pagina;
}

uint32_t recibir_valor_bloque(){
	uint32_t valor = 0;
	while (true) {
		int cod_op = recibir_operacion(fd_filesystem);
		switch (cod_op) {
		case VALOR_EN_BLOQUE_VUELTA:
			valor = recv_valor_en_bloque(fd_filesystem);
			break;
		}

		return valor;
	}
}

void cargar_pagina(int pid, int numero_pagina, int desplazamiento){
	//ver si la memoria esta llena (bitmap de marcos)
	bool flag_memoria_llena = true;
	int marco = 0;
	for(marco = 0; marco < cant_marcos; marco++){
		if(bitmap_marcos[marco] == '0'){
			flag_memoria_llena = false;
			break;
		}
	}
	t_pagina* pagina = buscar_pagina(pid, numero_pagina);

	//decirle a fs que me traiga lo que esta en el bloque
//	send_solicitud_valor_en_bloque(fd_filesystem, pagina->pos_swap);
	uint32_t valor = 1;//recibir_valor_bloque();
	int direccion = marco * tam_pagina + desplazamiento;
	//en ese caso, ejecutar algoritmo de reemplazo
	if(flag_memoria_llena){
		//algoritmo de reemplazo
		realizar_reemplazo(pagina, direccion, valor);
	}else{
		//sino - cargar pagina (asignarle un marco si no tiene y cambiarle el bit de presencia a 1)
		efectivizar_carga(marco, pagina, direccion, valor);
	}
	escribir_espacio_usuario(direccion, valor, pid, numero_pagina);
	contador_instante++;
}

void efectivizar_carga(int marco, t_pagina* pagina, int direccion, uint32_t valor){
	pagina->marco = marco;
	pagina->bit_presencia = 1;
	pagina->instante_de_referencia = contador_instante;
	pagina->direccion = direccion;
	bitmap_marcos[marco] = '1';
	list_add(paginas_en_memoria, pagina);
}

void descargar_pagina(t_pagina* pagina, int direccion){
	bitmap_marcos[pagina->marco] = '0';
	pagina->marco = -1;
	pagina->bit_presencia = 0;
	if(pagina->bit_modificado == 1){
		//TODO escribir en fs
		//uint32_t valor_a_escribir = leer_espacio_usuario(direccion, pagina->pid);
		//send_escribir_en_bloque(valor_a_escribir, fd_filesystem);

		pagina->bit_modificado = 0;
	}
	list_remove_element(paginas_en_memoria, pagina);
}

void realizar_reemplazo(t_pagina* pagina, int direccion, uint32_t valor){ //TODO
	if(!strcmp(algoritmo_reemplazo, "FIFO")){
		t_pagina* pagina_victima = list_remove(paginas_en_memoria, 0);
		int marco = pagina_victima->marco;
		descargar_pagina(pagina_victima, direccion);
		efectivizar_carga(marco, pagina, direccion, valor);
		log_info(logger, "REEMPLAZO - Marco: %d - Page Out: %d - %d - Page In: %d - %d", pagina->marco, pagina_victima->pid, pagina_victima->numpag, pagina->pid, pagina->numpag);
	}
	else if(!strcmp(algoritmo_reemplazo, "LRU")){
		void* _paginas_menor_referencia(t_pagina* pagina1, t_pagina* pagina2) {
		    return pagina1->instante_de_referencia <= pagina2->instante_de_referencia ? pagina1 : pagina2;
		}
		t_pagina* pagina_victima = list_get_minimum(paginas_en_memoria, (void*) _paginas_menor_referencia);
		int marco = pagina_victima->marco;
		descargar_pagina(pagina_victima, direccion);
		efectivizar_carga(marco, pagina, direccion, valor);
		log_info(logger, "REEMPLAZO - Marco: %d - Page Out: %d - %d - Page In: %d - %d", pagina->marco, pagina_victima->pid, pagina_victima->numpag, pagina->pid, pagina->numpag);
	}
}

uint32_t leer_espacio_usuario(int direccion, int pid) {
	uint32_t valor;
	log_info(logger, "PID: %d - Accion: LEER - Direccion fisica: %d", direccion, pid);
	pthread_mutex_lock(&mutex_memoria);
	memcpy(&valor, espacio_usuario + direccion, sizeof(uint32_t));
	pthread_mutex_unlock(&mutex_memoria);

	return valor;
}

void escribir_espacio_usuario(int direccion, uint32_t valor, int pid, int numero_pagina) {
	log_info(logger, "PID: %d - Accion: ESCRIBIR - Direccion fisica: %d", pid, direccion);

	pthread_mutex_lock(&mutex_memoria);
	memcpy(espacio_usuario + direccion, &valor, sizeof(int));
	pthread_mutex_unlock(&mutex_memoria);

	t_pagina* pagina = buscar_pagina(pid, numero_pagina);
	pagina->bit_modificado = 1;

}

void finalizar_proceso(t_pcb* pcb){
	//tabla de paginas
	bool _encontrar_pid(void* t) {
		return (((t_tdp*)t)->pid == pcb->pid);
	}

	t_tdp* tdp = list_find(tablas_de_paginas, _encontrar_pid);

	//borrarla de espacio usuario
	void pagina_iterate(t_pagina *p) {
		descargar_pagina(p, p->direccion);
	}

	list_iterate(tdp->paginas, (void*)pagina_iterate);
	//DESTRUIR! tdp
	void pagina_destroy(t_pagina *self) {
	    free(self);
	}

	list_destroy_and_destroy_elements(tdp->paginas, (void*)pagina_destroy);
	free(tdp);

	//destruir instrucciones
	bool _encontrar_pid_instrucciones(void* t) {
		return (((t_proceso_instrucciones*)t)->pid == pcb->pid);
	}

	t_proceso_instrucciones* pi = list_find(proceso_instrucciones, _encontrar_pid_instrucciones);
	void instrucciones_destroy(t_instruccion *self) {
//		free(self->param1);
//		free(self->param2);
		free(self);
	}
	list_destroy_and_destroy_elements(pi->instrucciones, (void*)instrucciones_destroy);
	free(pi);
}

