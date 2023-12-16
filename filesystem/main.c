#include "include/main.h"

t_log* logger;
t_config* config;

int main(void) {
	logger = log_create("filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_DEBUG);


//	int fd_memoria = 0;
//	int fd_cliente = 0;
	FILE* archivo_bloques;
	FILE* archivo_fat;
	config = iniciar_config();

	path_fat = config_get_string_value(config, "PATH_FAT");
	path_fcb = config_get_string_value(config, "PATH_FCB");
	path_bloques = config_get_string_value(config, "PATH_BLOQUES");
	ip_memoria = config_get_string_value(config, "IP_MEMORIA");

	pthread_mutex_init(&mutex_operaciones_pendientes, NULL);
	operaciones_pendientes = list_create();
	sem_init(&peticion_completada, 0, 2);
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
//	int cant_bloques_total = atoi(config_get_string_value(config, "CANT_BLOQUES_TOTAL"));
	RETARDO_ACCESO_FAT = config_get_int_value (config, "RETARDO_ACCESO_FAT");
	RETARDO_ACCESO_BLOQUE = config_get_int_value (config, "RETARDO_ACCESO_BLOQUE");
	cant_bloques_total = config_get_int_value(config, "CANT_BLOQUES_TOTAL");
	cant_bloques_swap = atoi(config_get_string_value(config, "CANT_BLOQUES_SWAP"));
	cant_bloques_fat = cant_bloques_total - cant_bloques_swap;
	tam_bloque = atoi(config_get_string_value(config, "TAM_BLOQUE"));
	tamanio_fat = cant_bloques_fat * tam_bloque;
	tamanio_swap = cant_bloques_swap * tam_bloque;
	tamanio_archivo_bloques = tamanio_swap + tamanio_fat;


	inicializar_archivo_fat(path_fat,cant_bloques_fat);
	inicializar_archivo_bloques(path_bloques);


	fd_memoria = crear_conexion(logger, ip_memoria, puerto_memoria);
	enviar_mensaje("Hola, soy un File System!", fd_memoria);


	server_fd = iniciar_servidor(puerto_escucha);
	log_info(logger, "FILESYSTEM LISTO...");

	iniciar_atencion_operaciones();

	// Conexion Kernel
//	pthread_t conexion_escucha;
//
//	pthread_create(&conexion_escucha, NULL, (void* )server_escuchar, NULL);
//	pthread_join(conexion_escucha, NULL);



	while(server_escuchar(logger, server_fd));


	while(experar_clientes(logger, server_fd));
	log_info(logger, "KERNEL CONECTADO A FS");


//	while(manejar_peticiones());


	config_destroy(config);
	fclose(archivo_bloques);
	fclose(archivo_fat);
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

void manejar_peticiones(){
	pthread_t hilo_peticion;
	pthread_create(&hilo_peticion, NULL, (void*) procesar_conexion, NULL);
	pthread_detach(hilo_peticion);

}

void iniciar_atencion_operaciones(){
	pthread_t hilo_peticiones;
	log_info(logger, "Inicio atencion de operaciones");

	pthread_create(&hilo_peticiones, NULL, (void*) atender_operaciones, NULL);
	pthread_detach(hilo_peticiones);
}

void atender_operaciones(){
	while(1){
		sem_wait(&cantidad_operaciones); // aca se queda esperando a que lleguen peticiones
		log_info(logger, "Hay peticion pendiente");
		t_operacion* operacion = list_pop_con_mutex(operaciones_pendientes, &mutex_operaciones_pendientes);
		realizar_operacion(operacion);
	}
}

int server_escuchar() {
	socket_cliente = esperar_cliente(logger, server_fd);
	if (socket_cliente != -1){
		//log_info(logger, "Hubo un error en la conexion del servidor");
		pthread_t hilo_conexion;
		pthread_create(&hilo_conexion, NULL, (void*) procesar_conexion, NULL);
		pthread_detach(hilo_conexion);
		return 1;
	}

	//procesar_conexion();
	return 0;
}


t_operacion* crear_operacion(codigo_operacion_fs cod_op, char* nombre_archivo, uint32_t buffer_escritura, int tamanio, int dir_fisica, int puntero, int cantidad_bloques_solicitados_swap, t_bloques_swap* bloques_ocupados_swap){
	t_operacion* operacion = malloc(sizeof(t_operacion));
	operacion->cod_op  = cod_op;
	operacion->nombre = nombre_archivo;
	operacion->buffer_escritura = buffer_escritura;
//	strcpy(operacion->nombre,nombre_archivo);
//	strcpy(operacion->buffer_escritura,buffer_escritura);
	operacion->tamanio = tamanio;
	operacion->dir_fisica = dir_fisica;
	operacion->puntero = puntero;
	operacion->cantidad_bloques_swap = cantidad_bloques_solicitados_swap;
	operacion->bloques_ocupados_swap = bloques_ocupados_swap;
	return operacion;
}


void procesar_conexion() {
	op_code cop;

	while (socket_cliente != -1) {
		//sem_wait(&peticion_completada);
        if (recv(socket_cliente, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
            log_info(logger, "El cliente se desconecto");
            return;
        }
//		int cop = recibir_operacion(socket_cliente);
		switch (cop) {
		case MENSAJE:
			recibir_mensaje(logger, socket_cliente);
			break;
		case FOPEN:
			t_peticion* peticion_open = recv_peticion(socket_cliente);
			char* nombre_archivo_open = strdup(peticion_open->nombre_archivo);
			log_info(logger,"Manejando FOPEN");
			t_operacion* op_open = crear_operacion(ABRIR_ARCHIVO_FS,nombre_archivo_open, 0, 0, 0, 0, 0, 0);
			list_push_con_mutex(operaciones_pendientes,op_open, &mutex_operaciones_pendientes);
			sem_post(&cantidad_operaciones);
			break;
		case FTRUNCATE:
//			t_list* parametros_truncate = recv_parametros(socket_cliente);
//			char* nombre_archivo_truncate = list_get(parametros_truncate, 0);
//			int* tamanio_truncate = list_get(parametros_truncate, 1);
			t_peticion_ftruncate* peticion_ftruncate = recv_peticion_f_truncate(socket_cliente);


			log_info(logger,"Manejando FTRUNCATE");
			t_operacion* op_truncate = crear_operacion(TRUNCAR_ARCHIVO_FS, peticion_ftruncate->nombre_archivo, 0,peticion_ftruncate->tamanio, 0, 0, 0, 0);
			list_push_con_mutex(operaciones_pendientes,op_truncate, &mutex_operaciones_pendientes);
			sem_post(&cantidad_operaciones);
			break;
		case FREAD:
			t_peticion_fread_fs* peticion_fread = recv_peticion_f_read_fs(socket_cliente);

			log_info(logger,"Manejando FREAD");
			t_operacion* op_read = crear_operacion(LEER_ARCHIVO_FS,peticion_fread->nombre_archivo, 0, 0, peticion_fread->direccion_fisica, peticion_fread->puntero, 0 , 0);
			list_push_con_mutex(operaciones_pendientes,op_read, &mutex_operaciones_pendientes);
			sem_post(&cantidad_operaciones);
			break;
		case FWRITE:
			t_peticion_fwrite_fs* peticion_fwrite = recv_peticion_f_write_fs(socket_cliente);


			log_info(logger,"Manejando FWRITE");
			t_operacion* op_write = crear_operacion(ESCRIBIR_ARCHIVO_FS, peticion_fwrite->nombre_archivo, 0, 0, peticion_fwrite->direccion_fisica, peticion_fwrite->puntero, 0, 0);
			list_push_con_mutex(operaciones_pendientes,op_write, &mutex_operaciones_pendientes);

			sem_post(&cantidad_operaciones);
			break;
		case INICIARPROCESO:
			t_list* parametros_iniciar_proceso = recv_parametros(socket_cliente);
			int* cantidad_bloques_solicitados = list_get(parametros_iniciar_proceso,0);

			log_info(logger,"Manejando INICIARPROCESO");
			t_operacion* op_iniciar_proceso = crear_operacion(INICIAR_PROCESO_FS, "", 0, 0, 0, 0, *cantidad_bloques_solicitados, 0);
			list_push_con_mutex(operaciones_pendientes,op_iniciar_proceso, &mutex_operaciones_pendientes);
			sem_post(&cantidad_operaciones);
			break;
		case FINALIZARPROCESO:
			t_list* parametros_finalizar_proceso = recv_parametros(socket_cliente);
			t_bloques_swap* bloques_ocupados_swap = list_get(parametros_finalizar_proceso,0);

			log_info(logger,"Manejando FINALIZARPROCESO");
			t_operacion* op_finalizar_proceso = crear_operacion(FINALIZAR_PROCESO_FS,  "", 0, 0, 0, 0, 0, bloques_ocupados_swap);
			list_push_con_mutex(operaciones_pendientes,op_finalizar_proceso, &mutex_operaciones_pendientes);
			sem_post(&cantidad_operaciones);
			break;
		default:
			log_error(logger, "Algo anduvo mal en el server");
			return;
		}
	}

	log_warning(logger, "El cliente se desconecto del server");
	return;
}


//void server_escuchar() {
//	server_name = "Filesystem";
//	socket_cliente = esperar_cliente(logger, server_name, fd_filesystem);
//
//	if (socket_cliente == -1){
//		log_info(logger, "Hubo un error en la conexion del %s", server_name);
//	}
//	procesar_conexion();
//}


// los valores de todo llegan por la operacion, aca se trata como si ya tuviera los valores, y mensajes de respuesta
void realizar_operacion(t_operacion* operacion){
	codigo_operacion_fs operacion_fs = operacion -> cod_op;
	switch(operacion_fs){
	case ABRIR_ARCHIVO_FS:
		log_info(logger, "fopen con nombre_archivo: %s", operacion->nombre);
		int resultado = abrir_archivo(operacion->nombre);
		if(resultado == 0){
			crear_archivo_fcb(operacion->nombre);
			send_finalizo_fopen(socket_cliente, 1);
		} else if(resultado != -1){
			send_finalizo_fopen(socket_cliente, 1);
		} else {
			send_finalizo_fopen(socket_cliente, 1);
		}
		break;
	case TRUNCAR_ARCHIVO_FS:
		log_info(logger, "ftruncate con nombre_archivo: %s y tamanio a truncar %d", operacion->nombre, operacion->tamanio);
		truncar_archivo(operacion->nombre, operacion->tamanio);
		send_finalizo_ftruncate(socket_cliente, 1);

		break;
	case LEER_ARCHIVO_FS:
		char* buffer_lectura = leer_archivo(operacion->nombre,operacion->dir_fisica,operacion->puntero);
		// PASAJE DE BUFFER A MEMORIA, VUELTA CON CONFIRMACION Y
		send_finalizo_fread(socket_cliente, 1);
		break;
	case ESCRIBIR_ARCHIVO_FS:
		//send_solicitud_lectura(operacion->dir_fisica, fd_memoria);
		//operacion->buffer_escritura = recv_valor_leido(fd_memoria);
		operacion->buffer_escritura = 1439852;
		escribir_archivo(operacion->nombre, operacion->dir_fisica, operacion->puntero,operacion->buffer_escritura,sizeof(operacion->buffer_escritura));
		send_finalizo_fwrite(socket_cliente, 1);
		break;
	case INICIAR_PROCESO_FS:
		log_info(logger, "iniciar_proceso con cantidad_bloques_swap: %d", operacion->cantidad_bloques_swap);
		uint32_t* lista_bloques_reservados = reservar_bloques_swap(operacion->cantidad_bloques_swap);
		int tamanio_lista = sizeof(uint32_t) * operacion->cantidad_bloques_swap;
		send_bloques_reservados(socket_cliente, lista_bloques_reservados, tamanio_lista);
		break;
	case FINALIZAR_PROCESO_FS:
		log_info(logger, "finalizar_proceso con cantidad de bloques a liberar: %ld", operacion->bloques_ocupados_swap->cantidad_bloques_a_liberar);
		liberar_bloques_swap(operacion->bloques_ocupados_swap);
		break;
	}
}

char* convertir_path_fcb(char* nombre_archivo_fcb){
	log_info(logger, "Entrando a convertir_path_fcb con nombre_archivo: %s", nombre_archivo_fcb);
	char* path = malloc(strlen("./fcbs/") + strlen(nombre_archivo_fcb) + strlen(".fcb") + 1); // +1 para el carácter nulo al final
	strcpy(path, "./fcbs/");
	strcat(path, nombre_archivo_fcb);
	strcat(path, ".fcb");
	return path;
}


// verificar si existe el fcb del archivo
// si existe devolver el tamanio, si no se informa que no existe
int abrir_archivo(char* nombre_archivo){ // o deberia revisar el path? creo q si
	char* path = convertir_path_fcb(nombre_archivo);
//	char* path = "./fcbs/consolas.fcb";
	t_config* archivo_fcb = config_create(path);

	if (archivo_fcb == NULL) {
		log_info(logger, "Archivo: %s no existe", nombre_archivo);
		//t_config* archivo_fcb = config_create(path); // ¿cuando creo el archivo?
		//archivo_fcb = crear_archivo_fcb(nombre_archivo);
		free(path);
		return 0;
	} else {
		log_info(logger, "Abrir Archivo: %s", nombre_archivo);
//		t_config* archivo_fcb = config_create(path);
		int tamanio_archivo = config_get_int_value(archivo_fcb,"TAMANIO_ARCHIVO");
		return tamanio_archivo;
	}
	return -1;
}

char* leer_archivo(char* nombre_archivo, int direccion_fisica, int puntero){

	log_info(logger,"Leer Archivo: %s - Puntero: %d - Memoria: %d", nombre_archivo, direccion_fisica, puntero);
	// El puntero pasaria por parametro, hay que ubicarlo en el primer byte del bloque a leer
	char* path = convertir_path_fcb(nombre_archivo);
	char* buffer_leido;
//	char* buffer_mensaje;
	t_config* archivo_fcb = config_create(path);
	int bloque_puntero_archivo = (puntero / tam_bloque);

	// el puntero es propio de cada archivo, por ende el puntero apunta por ejemplo al bloque 3 DEL ARCHIVO, no del fs
	int bloque_correspondiente = buscar_bloque_en_fat(archivo_fcb, puntero);

//	int bloque_real = buscar_bloque_en_fat(bloque_correspondiente, archivo_bloques);

	strcpy(buffer_leido,leer_datos_bloque_archivo(archivo_fcb, bloque_correspondiente));
	log_info(logger,"Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque FS: %d",nombre_archivo,bloque_puntero_archivo,(cant_bloques_swap + bloque_correspondiente));

	return buffer_leido;
//	send_datos_archivos_bloque(fd_memoria, buffer_leido, direccion_fisica);
	//enviar_mensaje(buffer_mensaje, fd_kernel);
	// ENVIAR A MEMORIA
	// se guarda la info leida y se envia a memoria empaquetada
	// memoria hace un handshake
	// luego se envia un mensajito a kernel diciendo que el traspaso de info fue exitoso
}



char* leer_datos_bloque_archivo(t_config* archivo_fcb, int bloque_a_leer){
	char* datos_leidos_bloque = malloc(tam_bloque);
	FILE* archivo_bloques = fopen(path_bloques, "rb+");


	if (archivo_bloques){
		usleep(RETARDO_ACCESO_BLOQUE * 1000);
	    fread(datos_leidos_bloque, tam_bloque, 1, archivo_bloques);
	}
	else{
	    puts("Something wrong reading from File.\n");
	}

	// o me conviene dejarlo abierto?
	fclose(archivo_bloques);


	return datos_leidos_bloque;
}

void escribir_datos_bloque_archivo(t_config* archivo_fcb, uint32_t bloque_a_escribir, uint32_t buffer_escritura){

//	char* datos_escritos_bloque = malloc(tamanio_bloque);
	FILE* archivo_bloques = fopen(path_bloques, "wb+");

	if (archivo_bloques){
		// en donde pongo el puntero? deberia ser al final de swap, osea tamanio_swap, mas donde esta ubicado el puntero
		fseek(archivo_bloques,tamanio_swap + (bloque_a_escribir * tam_bloque),SEEK_SET);
		usleep(RETARDO_ACCESO_BLOQUE * 1000);
		fwrite(&buffer_escritura, sizeof(uint32_t), 1, archivo_bloques);
	}
	else{
	    puts("Something wrong opening the File.\n");
	}

	// o me conviene dejarlo abierto?
	fclose(archivo_bloques);
//	free(datos_escritos_bloque);

}

void escribir_archivo(char* nombre_archivo, int direccion_fisica, int puntero, uint32_t buffer_a_escribir, int tamanio_a_escribir){
	//cuando se escribe un archivo, primero hay que truncarlo, modificar el fcb con el nuevo tamaño y escribir en los bloques
	// NO, YA ESTA TRUNCADO
	int bloque_puntero_archivo = (puntero / tam_bloque);
	char* path = convertir_path_fcb(nombre_archivo);
//	char* bufferEscrito;
	t_config* archivo_fcb = config_create(path);
//	int tamanio_actual_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
//	int tamanio_a_cambiar= tamanio_actual_archivo + tamanio_a_escribir;
	truncar_archivo(nombre_archivo,64); // ACA ESTOY TRUNCANDO PORQ NO LO HICE ANTES EN EL ARCHIVO PARA LA PRUEBA, NO FUNCA PARA SEGUIR EN KERNEL, POR ESO TRUNCO, PERO YA DEBERIA ESTAR

	// aca entra con el archivo ya truncado y busca el bloque del archivo al que apunta el puntero
	archivo_fcb = config_create(path);
	uint32_t bloque_correspondiente = buscar_bloque_en_fat(archivo_fcb, puntero);


	log_info(logger, "Escribir Archivo : %s - Puntero: %d - Memoria: %d", nombre_archivo, puntero, direccion_fisica);
	escribir_datos_bloque_archivo(archivo_fcb, bloque_correspondiente, buffer_a_escribir);
	log_info(logger,"Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque FS: %d",nombre_archivo,bloque_puntero_archivo,(cant_bloques_swap + bloque_correspondiente));
}


uint32_t  buscar_bloque_en_fat (t_config* archivo_fcb, int puntero){ // busca el bloque en la fat
	// tiene que buscar en el puntero de ese bloque

	int tamanio_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
	//int tamanio_bloque = config_get_int_value(config, "TAM_BLOQUE");
//	int cantidad_bloques_archivo = tamanio_archivo / tam_bloque;
	FILE* archivo_fat = fopen(path_fat,"rb");

	///// PASAR A T_LIST todo //////

	int bloque_puntero_archivo = (puntero / tam_bloque); // devuelve que a bloque apunta el puntero

	uint32_t numero_bloque_fat = obtener_bloque_puntero_en_fat(archivo_fcb,archivo_fat, bloque_puntero_archivo);

	// ahora ubico el puntero
	//list_add_all(lista_bloques_archivo,obtener_bloques_asignados(archivo_fcb,archivo_fat));
//	t_basta* bloque_fat = list_get(lista_bloques_archivo, bloque_puntero_archivo);

	//uint32_t numero_bloque_fat = bloque_fat->bloque_actual;


//	if (lista_bloques_archivo == NULL) {
//		return -1;
//	}

//	int numero_bloque_fat = -1; // Valor predeterminado para indicar un error o no encontrado
//	if (bloque_puntero_archivo >= 0 && bloque_puntero_archivo < cantidad_bloques_archivo) {
//		numero_bloque_fat = lista_bloques_archivo[bloque_puntero_archivo];
//	} else {
//		// Manejar el caso de índice fuera de límites
//		printf("Error: Indice fuera de limites.\n");
//    }
	fclose(archivo_fat);
//	free(bloque_fat);
//	free(lista_bloques_archivo);
	return numero_bloque_fat; // retorna el numero de bloque del archivo en donde esta el puntero (ya que me interesa el leer el bloque entero)
}

uint32_t obtener_bloque_puntero_en_fat(t_config* archivo_fcb,FILE* archivo_fat, int bloque_puntero_archivo) {
	uint32_t bloque_inicial = config_get_int_value(archivo_fcb, "BLOQUE_INICIAL");
	  //FILE* archivo_fat = fopen(path_fat, "rb"); YA LE LLEGA ABIERTO
//	int tamanio_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");

	uint32_t aux = 0;
	if (archivo_fat == NULL) {
	   perror("Error al abrir el archivo FAT para lectura binaria");
	   return 0;
	    }
	    uint32_t bloque_actual =  bloque_inicial;
	   // uint32_t bloque_actual = bloque_inicial;

	    int i = 0;
	    while (bloque_actual != 0) {

	    	if (i == bloque_puntero_archivo) {
	    		return bloque_actual;
	    	};

	        // Mover el puntero a la siguiente entrada en la tabla FAT
	        usleep(RETARDO_ACCESO_FAT * 1000);
	        fseek(archivo_fat, bloque_actual * sizeof(uint32_t), SEEK_SET);

	        fread(&(aux), sizeof(uint32_t), 1, archivo_fat);
			log_info(logger, "Acceso a FAT - Entrada: %d - Valor: %d", bloque_actual, aux);
			bloque_actual = aux;
	        i++;

	    }


	    return 0;
};


// Función para obtener la lista de bloques asignados a un archivo
t_list* obtener_bloques_asignados(t_config* archivo_fcb, FILE* archivo_fat) {
	t_list* lista_bloques = list_create();
	uint32_t bloque_inicial = config_get_int_value(archivo_fcb, "BLOQUE_INICIAL");
    //FILE* archivo_fat = fopen(path_fat, "rb"); YA LE LLEGA ABIERTO
	int tamanio_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
	int cantidad_bloques_asignados = tamanio_archivo / tam_bloque;
	int cantidad_bloques_restantes = cantidad_bloques_asignados;
	uint32_t aux = 0;
    if (archivo_fat == NULL) {
        perror("Error al abrir el archivo FAT para lectura binaria");
        return NULL;
    }
    t_basta* bloque_ayuda = malloc(sizeof(t_basta));
    bloque_ayuda->bloque_actual = bloque_inicial;
   // uint32_t bloque_actual = bloque_inicial;

    while (bloque_ayuda->bloque_actual != 0) {

//    	list_add(lista_bloques, bloque_ayuda);
    	//int* bloque_pos = list_get(lista_bloques,i);
    	//i++;
    	//log_warning(logger, "se encontro el :%d", *bloque_pos);
//        lista_bloques[tamanio++] = bloque_actual;
    	//cantidad_bloques_restantes--;
    	//if (i == bloque_puntero_archivo) {
    	//	return bloque_ayuda->bloque_actual;
    	//};

//    	if (cantidad_bloques_restantes == 0) {
//    	//	free(bloque_ayuda);
//    		return lista_bloques;
//    	}
        // Mover el puntero a la siguiente entrada en la tabla FAT
//        usleep(RETARDO_ACCESO_FAT * 1000);
//        fseek(archivo_fat, bloque_ayuda->bloque_actual * sizeof(uint32_t), SEEK_SET);
//        fread(&(bloque_ayuda->bloque_actual), sizeof(uint32_t), 1, archivo_fat);
//        aux = bloque_ayuda->bloque_actual;
//        free(bloque_ayuda);
//        t_basta* bloque_ayuda = malloc(sizeof(t_basta));
//        bloque_ayuda->bloque_actual = aux;
//        i++;

    }

    //fclose(archivo_fat);

    return lista_bloques;
}


// todo tengo que ver como obtiene el tamanio a cambiar
void truncar_archivo(char* nombre_archivo, int tamanio_a_cambiar) { // SUPUSE QUE EL TAMAÑO PUEDE SER NEGATIVO SI SE REDUZCO EL ARCHIVO
//	int tamanio_actual_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO"); NO LA USO
	char* path = convertir_path_fcb(nombre_archivo);
	t_config* archivo_fcb = config_create(path);
	if(tamanio_a_cambiar > 0){
		agrandar_archivo(archivo_fcb, tamanio_a_cambiar);
		log_info(logger, "Truncar archivo: %s - Tamanio: %d", nombre_archivo, tamanio_a_cambiar);

	} else  {
		if (tamanio_a_cambiar < 0){
		reducir_archivo(archivo_fcb, tamanio_a_cambiar);
		log_info(logger, "Truncar archivo: %s - Tamanio: %d", nombre_archivo, tamanio_a_cambiar);
		} else {
			log_info(logger, "No es necesario truncar el archivo %s", nombre_archivo);
		}
	}
	config_destroy(archivo_fcb);
}


void agrandar_archivo(t_config* archivo_fcb, int tamanio_a_agregar){

	// seteo el nuevo tamanio en el fcb
	//int tamanio_bloque = config_get_int_value(config, "TAMANIO_BLOQUE");
	// tengo que ver como hago para pasar de int a string

	// ahora tendria que reservar o asignar bloques
	asignar_bloques_a_archivo(archivo_fcb, tamanio_a_agregar);
}
void reducir_archivo(t_config* archivo_fcb, int tamanio_a_reducir){

		// seteo el nuevo tamanio en el fcb
		// ahora tendria que reservar o asignar bloques
		desasignar_bloques_a_archivo(archivo_fcb, tamanio_a_reducir);
}


void asignar_bloques_a_archivo(t_config* archivo_fcb,  int tamanio_a_agregar) {

	int cantidad_bloques_solicitados = (tamanio_a_agregar / tam_bloque);
	int tamanio_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
//	int cantidad_bloques_archivo = tamanio_archivo / tam_bloque;
//	int nuevo_tamanio = tamanio_archivo + tamanio_a_agregar;
    // el tamanio del archivo va de acuerdo a los bloques que tiene asignado
	int nuevo_tamanio = tamanio_archivo + tamanio_a_agregar;
    FILE* archivo_fat = fopen(path_fat, "rb+");
	// Obtener la lista de bloques asignados actualmente

    t_list* lista_bloques_actuales = obtener_bloques_asignados(archivo_fcb, archivo_fat);
    int tamanio_lista = list_size(lista_bloques_actuales);


    // Determinar desde qué bloque asignar (último bloque asignado o bloque inicial si no tiene asignados)
    uint32_t* bloque_asignar_desde;

    uint32_t bloque_inicial_asignacion = 0;

    if (list_is_empty(lista_bloques_actuales)) {
    	// Si no hay bloques asignados, asignar desde el bloque inicial configurado en el archivo FCB
    	bloque_inicial_asignacion = config_get_int_value(archivo_fcb, "BLOQUE_INICIAL");
    } else {
    	// Si ya hay bloques asignados, asignar desde el último bloque asignado
    	//bloque_asignar_desde = lista_bloques_actuales[cantidad_bloques_archivo - 1];
    	bloque_asignar_desde = list_get(lista_bloques_actuales, tamanio_lista-1);
    	bloque_inicial_asignacion = *bloque_asignar_desde;
    }



    // Buscar el primer bloque libre
    uint32_t bloque_libre = buscar_primer_bloque_libre_fat(archivo_fat);

    if(bloque_inicial_asignacion == 0) { // no tiene bloques asignados
    	char bloque_libre_str[12];  // Suficientemente grande para contener el valor máximo de un uint32_t
    	sprintf(bloque_libre_str, "%u", bloque_libre);
    	config_set_value(archivo_fcb, "BLOQUE_INICIAL", bloque_libre_str);
    	// escribe en el config y deberia ya asignarle el bloque libre a fat

    	uint32_t var_max = UINT32_MAX;
    	// SE BUSCA EL BLOQUE ENCONTRADO EN LA TABLA FAT,Y SE ESCRIBE EL EOF
    	fseek(archivo_fat, bloque_libre * sizeof(uint32_t), SEEK_SET);
    	usleep(RETARDO_ACCESO_FAT * 1000);
    	fwrite(&var_max, sizeof(uint32_t), 1, archivo_fat);

    	// ahora tengo que guardar este bloque en la lista de bloques asignados
    	list_add(lista_bloques_actuales, &bloque_libre);

    	log_info(logger,"Acceso FAT - Entrada: %d", bloque_libre);
    	// ya se le asigno un bloque, el inicial, entonces resto uno de los bloques que se necesitan
    	cantidad_bloques_solicitados--;

    	// Actualizar el bloque desde el cual asignar en la siguiente iteración
    	bloque_inicial_asignacion = bloque_libre;
    }

    if (bloque_libre == -1) {
        // Manejar el caso en que no hay bloques libres disponibles
        perror("No hay bloques libres disponibles");
        free(lista_bloques_actuales);
        return;
    }

    // Asignar bloques al archivo actualizando la tabla FAT

    if (archivo_fat == NULL) {
        perror("Error al abrir el archivo FAT para lectura y escritura binaria");
        free(lista_bloques_actuales);
        return;
    }

   //////////////////////////////////////////////////////////////////////////////// LLEGARA HASTA ACA ?????? ////// SIIIII
    // Actualizar la tabla FAT con la secuencia de bloques asignados
    int aux = 0;
    for (int i = 0; i < cantidad_bloques_solicitados; i++) {

    	 // Buscar el siguiente bloque libre
    	 bloque_libre = buscar_primer_bloque_libre_fat(archivo_fat);

    	 if (bloque_libre == -1) {
    	 // Manejar el caso en que no hay más bloques libres
    		 perror("No hay más bloques libres disponibles");
    		 break;
    	 }



    	// TENGO QUE ESCRIBIR EN EL BLOQUE_ASIGNAR_DESDE EL BLOQUE ENCONTRADO
    	fseek(archivo_fat, bloque_inicial_asignacion * sizeof(uint32_t), SEEK_SET);
    	usleep(RETARDO_ACCESO_FAT * 1000);
    	fwrite(&bloque_libre, sizeof(uint32_t), 1, archivo_fat);


    	uint32_t var_max = UINT32_MAX;
    	 // SE BUSCA EL BLOQUE ENCONTRADO EN LA TABLA FAT,Y SE ESCRIBE EL EOF
    	fseek(archivo_fat, bloque_libre * sizeof(uint32_t), SEEK_SET);
        usleep(RETARDO_ACCESO_FAT * 1000);
        fwrite(&var_max, sizeof(uint32_t), 1, archivo_fat);

        log_info(logger,"Acceso FAT - Entrada: %d - Valor: %d" , bloque_inicial_asignacion, bloque_libre);
        // Agregar el bloque a la lista de bloques asignados del archivo
        list_add(lista_bloques_actuales, &bloque_libre);



        bloque_inicial_asignacion = bloque_libre;  // Actualizar el bloque desde el cual asignar en la siguiente iteración
    }

    char nuevo_tamanio_str[12];  // Suficientemente grande para contener el valor máximo de un uint32_t
    sprintf(nuevo_tamanio_str, "%u", nuevo_tamanio);
    config_set_value(archivo_fcb, "TAMANIO_ARCHIVO", nuevo_tamanio_str);

    fclose(archivo_fat);

    // Liberar la memoria de la lista anterior de bloques asignados
    free(lista_bloques_actuales);

    // Guardar los cambios en el archivo FCB
    config_save(archivo_fcb);
}

void desasignar_bloques_a_archivo(t_config* archivo_fcb, int tamanio_a_reducir) {
    int cantidad_bloques_a_liberar = tamanio_a_reducir / tam_bloque;
    int tamanio_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
    int cantidad_bloques_archivo = tamanio_archivo / tam_bloque;
    FILE* archivo_fat = fopen(path_fat, "rb+");
    // Obtener la lista de bloques asignados actualmente
    uint32_t* lista_bloques_actuales = obtener_bloques_asignados(archivo_fcb, archivo_fat);

    if (lista_bloques_actuales == NULL) {
        // Manejar el error, por ejemplo, imprimir un mensaje de error
        perror("Error al obtener la lista de bloques asignados");
        return;
    }

    // Determinar desde qué bloque desasignar (último bloque asignado)
    int bloque_desasignar_desde;

    if (lista_bloques_actuales[0] != 0) {
        // Si ya hay bloques asignados, desasignar desde el último bloque asignado
        bloque_desasignar_desde = lista_bloques_actuales[cantidad_bloques_archivo - 1];
    } else {
        // Si no hay bloques asignados, no hay nada para desasignar
        free(lista_bloques_actuales);
        return;
    }

    // Desasignar bloques al archivo actualizando la tabla FAT
//    FILE* archivo_fat = fopen(path_fat, "r+b");

    if (archivo_fat == NULL) {
        perror("Error al abrir el archivo FAT para lectura y escritura binaria");
        free(lista_bloques_actuales);
        return;
    }

    // Desasignar la secuencia de bloques desde el final del archivo
    for (int i = 0; i < cantidad_bloques_a_liberar; i++) {
        fseek(archivo_fat, bloque_desasignar_desde * sizeof(uint32_t), SEEK_SET);
        usleep(RETARDO_ACCESO_BLOQUE * 1000);
        uint32_t bloque_libre;
        fread(&bloque_libre, sizeof(uint32_t), 1, archivo_fat);

        // Desasignar el bloque de la lista de bloques asignados del archivo
        if (cantidad_bloques_archivo - 1 - i >= 0) {
            lista_bloques_actuales[cantidad_bloques_archivo - 1 - i] = bloque_libre;
        }

        bloque_desasignar_desde = bloque_libre;  // Actualizar el bloque desde el cual desasignar en la siguiente iteración
    }

    fclose(archivo_fat);

    // Actualizar el tamaño del archivo en el archivo FCB
    int nuevo_tamanio = tamanio_archivo - tamanio_a_reducir;
    char nuevo_tamanio_str[12];
    snprintf(nuevo_tamanio_str, sizeof(nuevo_tamanio_str), "%u", nuevo_tamanio);
    config_set_value(archivo_fcb, "TAMANIO_ARCHIVO", nuevo_tamanio_str);

    // Liberar la memoria de la lista anterior de bloques asignados
    free(lista_bloques_actuales);

    // Guardar los cambios en el archivo FCB
    config_save(archivo_fcb);
}



bool archivo_sin_bloques(t_config* archivo_fcb){
	int primer_bloque = config_get_int_value(archivo_fcb, "PRIMER_BLOQUE");
	if(primer_bloque == 0){
		return true;
	} else return false;
}

uint32_t buscar_primer_bloque_libre_fat(FILE* archivo_fat) {
//    FILE* archivo_fat = fopen(path_fat, "rb"); siempre que la use ya va a estar abierto el archivo

    if (archivo_fat == NULL) {
        perror("Error al abrir el archivo FAT para lectura binaria");
        return -1; // Otra forma de indicar un error
    }

    uint32_t bloque_actual = 1; // porque no puede arrancar en el bloque 0
    uint32_t contenido_bloque_actual;
    fseek(archivo_fat, bloque_actual * sizeof(uint32_t), SEEK_SET); // se setea el puntero en el primer bloque despues del bloque 0
    usleep(RETARDO_ACCESO_FAT * 1000);
    while (fread(&contenido_bloque_actual, sizeof(uint32_t), 1, archivo_fat) == 1) { // se recorre la fat leyendo bloeuq a bloque

        if (contenido_bloque_actual == 0) {
            // Encontramos un bloque libre
//            fclose(archivo_fat); NO CIERRA ACA
            return bloque_actual;
        }
        bloque_actual++; // pasa al siguiente bloque y setea en el primer byte el puntero
        fseek(archivo_fat, bloque_actual * sizeof(uint32_t), SEEK_SET);
        usleep(RETARDO_ACCESO_FAT * 1000);
    }

//    fclose(archivo_fat); NO CIERROOOO

    // No se encontraron bloques libres
    return -1; // Otra forma de indicar que no hay bloques libres
}

t_config* crear_archivo_fcb(char* nombre_archivo){
	//segun el enunciado conviene hacerlo como si fuera un config

	char* aux_nombre = malloc(strlen(nombre_archivo) + 1);
	strcpy(aux_nombre, nombre_archivo);
	char* path = convertir_path_fcb(nombre_archivo);

	FILE* archivo_fcb_txt = fopen(path, "w");

	fprintf(archivo_fcb_txt, "NOMBRE_ARCHIVO=\n");
	fprintf(archivo_fcb_txt, "TAMANIO_ARCHIVO=\n");
	fprintf(archivo_fcb_txt, "BLOQUE_INICIAL=\n");

	fclose(archivo_fcb_txt);

	t_config* archivo_fcb = config_create(path);
	if (archivo_fcb == NULL) {
		// Imprime un mensaje de error si config_create falla
		fprintf(stderr, "Error al abrir el archivo FCB: %s\n", path);
		free(path);
		free(aux_nombre);
	return NULL;
	}
	config_set_value(archivo_fcb,"NOMBRE_ARCHIVO",aux_nombre);
	config_set_value(archivo_fcb,"TAMANIO_ARCHIVO","0");
	config_set_value(archivo_fcb,"BLOQUE_INICIAL","0");

	config_save(archivo_fcb);

	free(path);
	free(aux_nombre);

	log_info(logger, "Crear Archivo: %s ", nombre_archivo);

	return archivo_fcb;
}


void inicializar_archivo_fat(char* path_fat, int cantidad_bloques_fat) {
    // Abrir el archivo en modo de escritura binaria
    FILE* archivo_fat = fopen(path_fat, "wb");

    // Crear un array de uint32_t con el tamaño deseado
    uint32_t* entradas = calloc(cantidad_bloques_fat, sizeof(uint32_t)); // Inicializa con 0 automáticamente

    if (archivo_fat == NULL) {
        log_error(logger, "Error al abrir el archivo FAT para escritura binaria");
        free(entradas);
        return;
    }

    // Escribir el array al archivo
    usleep(RETARDO_ACCESO_FAT * 1000);
    fwrite(entradas, sizeof(uint32_t), cantidad_bloques_fat, archivo_fat);

    log_info(logger, "SE CREO E INICIALIZO EL ARCHIVO FAT");

    // Cerrar el archivo y liberar la memoria
    fclose(archivo_fat);
    free(entradas);
}

uint32_t obtener_siguiente_bloque_fat(char* path_fat, uint32_t bloque_actual) {
    FILE* archivo_fat = fopen(path_fat, "rb");

    if (archivo_fat == NULL) {
        perror("Error al abrir el archivo FAT para lectura binaria");
        return 0;  // Devolver 0 en caso de error
    }

    // Mover el puntero a la entrada correspondiente en la tabla FAT
    fseek(archivo_fat, bloque_actual * sizeof(uint32_t), SEEK_SET);
    usleep(RETARDO_ACCESO_FAT * 1000);
    uint32_t bloque_siguiente;
    fread(&bloque_siguiente, sizeof(uint32_t), 1, archivo_fat);

    fclose(archivo_fat);

    return bloque_siguiente;
}


// TANTO PARA CREAR COMO PARA ABRIR UN ARCHIVO NO TIENE NINGUNA IMPLICANCIA EN EL ARCHIVO DE BLOQUES, ESTE SOLO GUARDA DATOS
// Y ABRIR CREO QUE VA MAS POR EL LADO DE LOS PERMISOS Y ESAS COSAS

// LAS PRIMERAS OPERACIONES QUE COMIENZAN A TENER IMPLICANCIA DIRECTA EN EL ARCHIVO DE BLOQUES SON LEER, ESCRIBIR Y TRUNCAR
// CABE ACLARAR QUE CADA VEZ QUE SE ESCRIBE HAY QUE TRUNCAR HASTA EL TAMAÑO QUE SE NECESITE, LUEGO TENGO QUE VER COMO REPRESENTO LOS BLOQUES
// A PARTIR DEL ARCHIVO DE BLOQUES
// TAMBIEN ACLARO QUE EN LA ASIGNACION DE BLOQUES, QUE SE VA A DAR PRIMERO BUSCANDO UN BLOQUE INICIAL Y LUEGO ASIGNANDO CUALQUIERA DE
// LOS BLOQUES SIGUIENTES


// primero deberia crear el archivo de bloques en caso de que no este en el path, es un archivo binario
void inicializar_archivo_bloques(char* path){

	//int tamanio_archivo_bloques = cant_bloques_total*tam_bloque;
	FILE* archivo_bloques = fopen(path,"wb+");
	// deberia setear todas las entradas, tanto de swap como de fat en 0;
	uint32_t* entradas = calloc(tamanio_archivo_bloques, sizeof(uint32_t)); // Inicializa con 0 automáticamente
//	fseek(archivo_bloques,0, SEEK_SET);

	usleep(RETARDO_ACCESO_BLOQUE * 1000);
	//fwrite(entradas, sizeof(uint32_t), tamanio_archivo_bloques, archivo_bloques);
	fwrite(entradas, sizeof(uint32_t), cant_bloques_total * 4, archivo_bloques);

//	for(int i = 0; i < tamanio_archivo_bloques; i++) {
//		fwrite("0", sizeof(uint32_t), 1, archivo_bloques);
//	}

    log_info(logger, "SE CREO E INICIALIZO EL ARCHIVO DE BLOQUES");

	fclose(archivo_bloques);
};

uint32_t* reservar_bloques_swap(int cantidad_bloques_solicitados){
	uint32_t* bloques_reservados = malloc(cantidad_bloques_solicitados * sizeof(uint32_t));
	char* path_archivo_bloques = config_get_string_value(config, "PATH_BLOQUES");
	int tamanio_bloque = config_get_int_value(config, "TAM_BLOQUE");
	FILE* archivo_bloques = fopen(path_archivo_bloques, "wb+") ;
	uint32_t bloque_actual = 0;
	uint32_t i = 0;
	while(cantidad_bloques_solicitados != 0) {
		bloque_actual = buscar_primer_bloque_libre_swap(archivo_bloques);
		fwrite("\0", sizeof(uint32_t), tamanio_bloque, archivo_bloques);
		bloques_reservados[i] = bloque_actual;
		cantidad_bloques_solicitados--;
	}
	fclose(archivo_bloques);
	return bloques_reservados;
}
void liberar_bloques_swap(t_bloques_swap* lista_bloques_a_liberar){
	size_t longitud_lista = lista_bloques_a_liberar->cantidad_bloques_a_liberar;
	char* path_archivo_bloques = config_get_string_value(config, "PATH_BLOQUES");
	int tamanio_bloque = config_get_int_value(config, "TAM_BLOQUE");

	FILE* archivo_bloques = fopen(path_archivo_bloques, "wb+") ;
	int i = 0;
	// comienza al principio de swap
	uint32_t bloque_a_liberar = 0;
	while(longitud_lista != 0) {
		bloque_a_liberar = lista_bloques_a_liberar->array_bloques[i];
		// liberar_bloque
		// tengo que ubicar el puntero al principio del bloque que se quiere liberar
		fseek(archivo_bloques, bloque_a_liberar * sizeof(uint32_t), SEEK_SET);
		usleep(RETARDO_ACCESO_BLOQUE * 1000);
		fwrite("0", sizeof(uint32_t), tamanio_bloque, archivo_bloques);
		i++;
		longitud_lista--;
	}
	free(lista_bloques_a_liberar->array_bloques);

	fclose(archivo_bloques);
}

uint32_t buscar_primer_bloque_libre_swap(FILE* archivo_bloques) {

    if (archivo_bloques == NULL) {
        perror("Error al abrir el archivo FAT para lectura binaria");
        return -1; // Otra forma de indicar un error
    }

    uint32_t bloque_actual = 0;
    fseek(archivo_bloques, bloque_actual * sizeof(uint32_t), SEEK_SET);
    usleep(RETARDO_ACCESO_BLOQUE * 1000);
    while (fread(&bloque_actual, sizeof(uint32_t), 1, archivo_bloques) == 1) { // no se como hacer para que no pase del tamanio de la swap (y que asigne bloques de FAT)
        // asumo que siempre va a haber un bloque libre
    	if (bloque_actual == 0) {
            // Encontramos un bloque libre
            return bloque_actual;
        }

        fseek(archivo_bloques, bloque_actual * sizeof(uint32_t), SEEK_SET);
        usleep(RETARDO_ACCESO_BLOQUE * 1000);
    }

    fclose(archivo_bloques);

    // No se encontraron bloques libres
    return -1; // Otra forma de indicar que no hay bloques libres
}





