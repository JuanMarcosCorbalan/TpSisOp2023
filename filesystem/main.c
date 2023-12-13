#include "include/main.h"

t_log* logger;
t_config* config;

int main(void) {
	logger = log_create("filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_DEBUG);

	char* ip;
	char* puerto_escucha;
	char* puerto_memoria;
//	int fd_memoria = 0;
//	int fd_cliente = 0;
	FILE* archivo_bloques;
	FILE* archivo_fat;
	config = iniciar_config();

	char* path_fat = config_get_string_value(config, "PATH_FAT");
	char* path_fcb = config_get_string_value(config, "PATH_FCB");
	char* path_bloques = config_get_string_value(config, "PATH_BLOQUES");


	ip = config_get_string_value(config, "IP");
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	int cant_bloques_total = atoi(config_get_string_value(config, "CANT_BLOQUES_TOTAL"));
	int cant_bloques_swap = atoi(config_get_string_value(config, "CANT_BLOQUES_SWAP"));
	int cant_bloques_fat = cant_bloques_total - cant_bloques_swap;
	int tam_bloque = atoi(config_get_string_value(config, "TAM_BLOQUE"));
	int tamanio_fat = cant_bloques_fat * tam_bloque;

	int tamanio_swap = cant_bloques_swap * tam_bloque;
	int tamanio_archivo_bloques = tamanio_swap + tamanio_fat;


	fd_memoria = crear_conexion(logger, ip, puerto_memoria);
	enviar_mensaje("Hola, soy un File System!", fd_memoria);


	int server_fd = iniciar_servidor(puerto_escucha);
	log_info(logger, "FILESYSTEM LISTO...");

	iniciar_atencion_operaciones();

	// Conexion Kernel
	pthread_t conexion_escucha;
	pthread_create(&conexion_escucha, NULL, (void*) server_escuchar, NULL);
	pthread_join(conexion_escucha, NULL);



//	while(experar_clientes(logger, server_fd));
//	log_info(logger, "KERNEL CONECTADO A FS");


//	while(manejar_peticiones());


	// esto deberia ir en manejar solicitudes? o antes?
	crear_archivo_fat(path_fat, cant_bloques_fat);
	t_config* archivo_fcb = crear_archivo_fcb("hola");

	archivo_bloques = fopen(path_bloques, "wb+");
	if (archivo_bloques == NULL) {
		crear_archivo_bloques(path_bloques);
		log_info(logger, "No se encontro el archivo de bloques, se creo uno nuevo");
	}

	archivo_fat = fopen(path_fat, "wb+");
	if (archivo_fat == NULL) {
		crear_archivo_fat(path_fat,cant_bloques_fat);
		log_info(logger, "No se encontro el archivo de fat, se creo uno nuevo");
	} else {
		log_info(logger, "Archivo de fat existente");
	}

	fclose(archivo_bloques);
	config_destroy(config);
	config_destroy(archivo_fcb);
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
		t_operacion* operacion = list_pop_con_mutex(operaciones_pendientes, mutex_operaciones_pendientes);
		realizar_operacion(operacion);
	}
}

void server_escuchar() {
	socket_cliente = esperar_cliente(logger, socket_servidor);

	if (socket_cliente == -1){
		log_info(logger, "Hubo un error en la conexion del servidor");
	}
	procesar_conexion();
}


t_operacion* crear_operacion(codigo_operacion_fs cod_op, char* nombre_archivo, uint32_t* buffer_escritura, int tamanio, int dir_fisica, int puntero, int cantidad_bloques_solicitados_swap, t_bloques_swap* bloques_ocupados_swap){
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
        if (recv(socket_cliente, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
            log_info(logger, "El cliente se desconecto");
            return;
        }
		switch (cop) {
		case MENSAJE:
			recibir_mensaje(logger, socket_cliente);
			break;
		case FOPEN:
			t_list* parametros_fopen = recv_parametros(socket_cliente);
			char* nombre_archivo_fopen = list_get (parametros_fopen, 0);
			log_info(logger,"Manejando FOPEN");
			t_operacion* op_open = crear_operacion(ABRIR_ARCHIVO_FS,nombre_archivo_fopen, 0, 0, 0, 0, 0, 0);
			list_push_con_mutex(operaciones_pendientes,op_open, mutex_operaciones_pendientes);
			sem_post(&cantidad_operaciones);
			break;
		case FTRUNCATE:
			t_list* parametros_truncate = recv_parametros(socket_cliente);
			char* nombre_archivo_truncate = list_get(parametros_truncate, 0);
			int* tamanio_truncate = list_get(parametros_truncate, 1);
			log_info(logger,"Manejando FTRUNCATE");
			t_operacion* op_truncate = crear_operacion(TRUNCAR_ARCHIVO_FS, nombre_archivo_truncate, 0, *tamanio_truncate, 0, 0, 0, 0);
			list_push_con_mutex(operaciones_pendientes,op_truncate, mutex_operaciones_pendientes);
			sem_post(&cantidad_operaciones);
			break;
		case FREAD:
			t_list* parametros_read = recv_parametros(socket_cliente);
			char* nombre_archivo_read = list_get(parametros_read, 0);
			int* dir_fisica_read = list_get(parametros_read, 1);
			int* tamanio_read = list_get(parametros_read, 2);
			int* puntero_read = list_get(parametros_read, 3);

			log_info(logger,"Manejando FREAD");
			t_operacion* op_read = crear_operacion(LEER_ARCHIVO_FS, nombre_archivo_read, 0,  *tamanio_read, *dir_fisica_read, *puntero_read, 0 , 0); // TODO
			list_push_con_mutex(operaciones_pendientes,op_read, mutex_operaciones_pendientes);
			sem_post(&cantidad_operaciones);
			break;
		case FWRITE:
			t_list* parametros_write = recv_parametros(socket_cliente);
			char* nombre_archivo_write = list_get(parametros_write, 0);
			int* dir_fisica_write = list_get(parametros_write, 1);
			int* tamanio_write = list_get(parametros_write, 2);
			int* puntero_write = list_get(parametros_write, 3);

			log_info(logger,"Manejando FWRITE");
			t_operacion* op_write = crear_operacion(ESCRIBIR_ARCHIVO_FS, nombre_archivo_write, 0, *tamanio_write, *dir_fisica_write, *puntero_write, 0, 0);
			list_push_con_mutex(operaciones_pendientes,op_write, mutex_operaciones_pendientes);

			sem_post(&cantidad_operaciones);
			break;
		case INICIARPROCESO:
			t_list* parametros_iniciar_proceso = recv_parametros(socket_cliente);
			int* cantidad_bloques_solicitados = list_get(parametros_iniciar_proceso,0);

			log_info(logger,"Manejando INICIARPROCESO");
			t_operacion* op_iniciar_proceso = crear_operacion(INICIAR_PROCESO_FS, "", 0, 0, 0, 0, *cantidad_bloques_solicitados, 0);
			list_push_con_mutex(operaciones_pendientes,op_iniciar_proceso, mutex_operaciones_pendientes);
			sem_post(&cantidad_operaciones);
			break;
		case FINALIZARPROCESO:
			t_list* parametros_finalizar_proceso = recv_parametros(socket_cliente);
			t_bloques_swap* bloques_ocupados_swap = list_get(parametros_finalizar_proceso,0);

			log_info(logger,"Manejando FINALIZARPROCESO");
			t_operacion* op_finalizar_proceso = crear_operacion(FINALIZAR_PROCESO_FS,  "", 0, 0, 0, 0, 0, bloques_ocupados_swap);
			list_push_con_mutex(operaciones_pendientes,op_finalizar_proceso, mutex_operaciones_pendientes);
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
			enviar_mensaje("Archivo no existente, se creara el archivo", socket_cliente);
			crear_archivo_fcb(operacion->nombre);
		} else {
			enviar_mensaje("Archivo abierto con exito", socket_cliente);
		}
		break;
	case TRUNCAR_ARCHIVO_FS:
		truncar_archivo(operacion->nombre, operacion->tamanio, path_fat);
		enviar_mensaje("Archivo truncado", socket_cliente);
		break;
	case LEER_ARCHIVO_FS:
		leer_archivo(operacion->nombre,operacion->dir_fisica,operacion->puntero,path_fat);
		break;
	case ESCRIBIR_ARCHIVO_FS:
		send_solicitud_lectura(operacion->dir_fisica, fd_memoria);
		operacion->buffer_escritura = recv_valor_leido(fd_memoria);
		escribir_archivo(operacion->nombre, operacion->dir_fisica, operacion->puntero,operacion->buffer_escritura,sizeof(operacion->buffer_escritura), path_fat);
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
	if (path == NULL) {
		log_info(logger, "Archivo: %s no existe", nombre_archivo);
		//t_config* archivo_fcb = config_create(path); // ¿cuando creo el archivo?
		//archivo_fcb = crear_archivo_fcb(nombre_archivo);
		free(path);
		return 0;
	} else {
		log_info(logger, "Abrir Archivo: %s", nombre_archivo);
		t_config* archivo_fcb = config_create(path);
		int tamanio_archivo = config_get_int_value(archivo_fcb,"TAMANIO_ARCHIVO");
		return tamanio_archivo;
	}
}

void leer_archivo(char* nombre_archivo, int direccion_fisica, int puntero, char* path_fat){

	// El puntero pasaria por parametro, hay que ubicarlo en el primer byte del bloque a leer
	char* path = convertir_path_fcb(nombre_archivo);
	char* buffer_leido;
	char* buffer_mensaje;
	t_config* archivo_fcb = config_create(path);

	// el puntero es propio de cada archivo, por ende el puntero apunta por ejemplo al bloque 3 DEL ARCHIVO, no del fs
	int bloque_correspondiente = buscar_bloque_en_fat(archivo_fcb, puntero, path_fat);

//	int bloque_real = buscar_bloque_en_fat(bloque_correspondiente, archivo_bloques);

	buffer_leido =  leer_datos_bloque_archivo(archivo_fcb, bloque_correspondiente);

//	send_datos_archivos_bloque(fd_memoria, buffer_leido, direccion_fisica);
	recibir_mensaje(logger,fd_memoria); // no se como hacer para que verifique que el mensaje que manda memoria es un OK
	buffer_mensaje = "traspaso de informacion leida con exito";
	//enviar_mensaje(buffer_mensaje, fd_kernel);
	// ENVIAR A MEMORIA
	// se guarda la info leida y se envia a memoria empaquetada
	// memoria hace un handshake
	// luego se envia un mensajito a kernel diciendo que el traspaso de info fue exitoso
}



char* leer_datos_bloque_archivo(t_config* archivo_fcb, int bloque_a_leer){
	int tamanio_bloque = config_get_int_value(archivo_fcb, "TAMANIO_BLOQUE");
	char* datos_leidos_bloque = malloc(tamanio_bloque);
	FILE* archivo_bloques = fopen("./ArchivoDeBloques", "rb+");
	int tamanio_buffer = strlen(datos_leidos_bloque) + 1;

	if (archivo_bloques){
		usleep(RETARDO_ACCESO_BLOQUE * 1000);
	    fread(datos_leidos_bloque, tamanio_buffer, 1, archivo_bloques);
	}
	else{
	    puts("Something wrong reading from File.\n");
	}

	// o me conviene dejarlo abierto?
	fclose(archivo_bloques);


	return datos_leidos_bloque;
}

void escribir_datos_bloque_archivo(t_config* archivo_fcb, int bloque_a_escribir, uint32_t* buffer_escritura){
	int tamanio_bloque = config_get_int_value(archivo_fcb, "TAMANIO_BLOQUE");
	char* datos_escritos_bloque = malloc(tamanio_bloque);
	FILE* archivo_bloques = fopen("./ArchivoDeBloques", "wb+");
	int tamanio_buffer = sizeof(uint32_t);

	if (archivo_bloques){
		usleep(RETARDO_ACCESO_BLOQUE * 1000);
		fwrite(buffer_escritura, tamanio_buffer, 1, archivo_bloques);
	}
	else{
	    puts("Something wrong opening the File.\n");
	}

	// o me conviene dejarlo abierto?
	fclose(archivo_bloques);
	free(buffer_escritura);
	free(datos_escritos_bloque);

}

void escribir_archivo(char* nombre_archivo, int direccion_fisica, int puntero, uint32_t* buffer_a_escribir, int tamanio_a_escribir,const char* path_fat){
	// primero se recibe en el buffer lo que se va a escribir (y oh sorpresa, despues lo hago :))
	//cuando se escribe un archivo, primero hay que truncarlo, modificar el fcb con el nuevo tamaño y escribir en los bloques

	char* path = convertir_path_fcb(nombre_archivo);
//	char* bufferEscrito;
	t_config* archivo_fcb = config_create(path);
	int tamanio_actual_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
	int tamanio_a_cambiar= tamanio_actual_archivo + tamanio_a_escribir;
	int bloque_correspondiente = buscar_bloque_en_fat(archivo_fcb, puntero, path_fat);
	truncar_archivo(nombre_archivo,tamanio_a_cambiar,path_fat); // mismo en truncar_archivo se ve si hay que ampliar, reducir o no?
	// hay que ver cuando se trunca
	escribir_datos_bloque_archivo(archivo_fcb, bloque_correspondiente, buffer_a_escribir);

}


uint32_t  buscar_bloque_en_fat (t_config* archivo_fcb, int puntero,const char* path_fat){ // busca el bloque en la fat
	// tiene que buscar en el puntero de ese bloque
	int tamanio_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
	int tamanio_bloque = config_get_int_value(config, "TAM_BLOQUE");
	int cantidad_bloques_archivo = tamanio_archivo / tamanio_bloque;
	uint32_t* lista_bloques_archivo = obtener_bloques_asignados(path_fat, archivo_fcb);
	int bloque_puntero_archivo = (puntero / tamanio_bloque) + 1; // porq la division me va a dar la parte entera y por ejemplo si me da el bloque 2.3, seria 0.3 del tercero
	// ahora ubico el puntero
	if (lista_bloques_archivo == NULL) {
		return -1;
	}

	int numero_bloque_fat = -1; // Valor predeterminado para indicar un error o no encontrado
	if (bloque_puntero_archivo >= 0 && bloque_puntero_archivo < cantidad_bloques_archivo) {
		numero_bloque_fat = lista_bloques_archivo[bloque_puntero_archivo];
	} else {
		// Manejar el caso de índice fuera de límites
		printf("Error: Indice fuera de limites.\n");
    }

	free(lista_bloques_archivo);
	return numero_bloque_fat; // retorna el numero de bloque del archivo en donde esta el puntero (ya que me interesa el leer el bloque entero)
}


// Función para obtener la lista de bloques asignados a un archivo
uint32_t* obtener_bloques_asignados(const char* path_fat, t_config* archivo_fcb) {
	uint32_t bloque_inicial = config_get_int_value(archivo_fcb, "BLOQUE_INICIAL");
    FILE* archivo_fat = fopen(path_fat, "rb");

    if (archivo_fat == NULL) {
        perror("Error al abrir el archivo FAT para lectura binaria");
        return NULL;
    }

    uint32_t* lista_bloques = NULL;
    int capacidad = 0;
    int tamano = 0;

    uint32_t bloque_actual = bloque_inicial;

    while (bloque_actual != 0) {

        // Agregar el bloque actual a la lista
        if (tamano >= capacidad) {
            // Aumentar la capacidad si es necesario
            capacidad = (capacidad == 0) ? 1 : capacidad * 2;
            lista_bloques = realloc(lista_bloques, capacidad * sizeof(uint32_t));

            if (lista_bloques == NULL) {
                perror("Error al asignar memoria");
                fclose(archivo_fat);
                return NULL;
            }
        }
        lista_bloques[tamano++] = bloque_actual;

        // Mover el puntero a la siguiente entrada en la tabla FAT
        usleep(RETARDO_ACCESO_FAT * 1000);
        fseek(archivo_fat, bloque_actual * sizeof(uint32_t), SEEK_SET);
        fread(&bloque_actual, sizeof(uint32_t), 1, archivo_fat);
    }

    fclose(archivo_fat);

    return lista_bloques;
}


// todo tengo que ver como obtiene el tamanio a cambiar
void truncar_archivo(char* nombre_archivo, int tamanio_a_cambiar,const char* path_fat) { // SUPUSE QUE EL TAMAÑO PUEDE SER NEGATIVO SI SE REDUZCO EL ARCHIVO
//	int tamanio_actual_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO"); NO LA USO
	char* path = convertir_path_fcb(nombre_archivo);
	t_config* archivo_fcb = config_create(path);
	if(tamanio_a_cambiar > 0){
		agrandar_archivo(archivo_fcb, tamanio_a_cambiar, path_fat);
		log_info(logger, "Truncar archivo: %s - Tamanio: %d", nombre_archivo, tamanio_a_cambiar);

	} else  {
		if (tamanio_a_cambiar < 0){
		reducir_archivo(archivo_fcb, tamanio_a_cambiar, path_fat);
		log_info(logger, "Truncar archivo: %s - Tamanio: %d", nombre_archivo, tamanio_a_cambiar);
		} else {
			log_info(logger, "No es necesario truncar el archivo %s", nombre_archivo);
		}
	}
}


void agrandar_archivo(t_config* archivo_fcb, int tamanio_a_agregar,const char* path_fat){

	// seteo el nuevo tamanio en el fcb
	int tamanio_bloque = config_get_int_value(config, "TAMANIO_BLOQUE");
	// tengo que ver como hago para pasar de int a string

	// ahora tendria que reservar o asignar bloques
	asignar_bloques_a_archivo(path_fat,archivo_fcb, tamanio_a_agregar, tamanio_bloque);
}
void reducir_archivo(t_config* archivo_fcb, int tamanio_a_reducir,const char* path_fat){
	// seteo el nuevo tamanio en el fcb
		int tamanio_actual_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
		int tamanio_bloque = config_get_int_value(config, "TAMANIO_BLOQUE");
		// ahora tendria que reservar o asignar bloques
		desasignar_bloques_a_archivo(path_fat, archivo_fcb, tamanio_a_reducir, tamanio_bloque);
}


void asignar_bloques_a_archivo(const char* path_fat, t_config* archivo_fcb,  int tamanio_a_agregar, int tamanio_bloque) {

	int cantidad_bloques_solicitados = tamanio_a_agregar / tamanio_bloque;
    int tamanio_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
	int cantidad_bloques_archivo = tamanio_archivo / tamanio_bloque;
	int nuevo_tamanio = tamanio_archivo + tamanio_a_agregar;
    FILE* archivo_fat = fopen(path_fat, "r+b");

	// Obtener la lista de bloques asignados actualmente
    uint32_t* lista_bloques_actuales = obtener_bloques_asignados(path_fat, archivo_fcb);


    if (lista_bloques_actuales == NULL) {
        // Manejar el error, por ejemplo, imprimir un mensaje de error
        perror("Error al obtener la lista de bloques asignados");
        return;
    }

    // Determinar desde qué bloque asignar (último bloque asignado o bloque inicial si no tiene asignados)
    int bloque_asignar_desde;

    if (lista_bloques_actuales[0] != 0) {
        // Si ya hay bloques asignados, asignar desde el último bloque asignado
        bloque_asignar_desde = lista_bloques_actuales[cantidad_bloques_archivo - 1];
    } else {
        // Si no hay bloques asignados, asignar desde el bloque inicial configurado en el archivo FCB
        bloque_asignar_desde = config_get_int_value(archivo_fcb, "BLOQUE_INICIAL");
    }
    // Buscar el primer bloque libre
    uint32_t bloque_libre = buscar_primer_bloque_libre_fat(archivo_fat);

    if(bloque_asignar_desde == 0) {
    	char bloque_libre_str[12];  // Suficientemente grande para contener el valor máximo de un uint32_t
    	sprintf(bloque_libre_str, "%u", bloque_libre);
    	config_set_value(archivo_fcb, "BLOQUE_INICIAL", bloque_libre_str);
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

    // Actualizar la tabla FAT con la secuencia de bloques asignados
    for (int i = 0; i < cantidad_bloques_solicitados; i++) {
        fseek(archivo_fat, bloque_asignar_desde * sizeof(uint32_t), SEEK_SET);
        usleep(RETARDO_ACCESO_FAT * 1000);
        fwrite(&bloque_libre, sizeof(uint32_t), 1, archivo_fat);

        // Agregar el bloque a la lista de bloques asignados del archivo
        lista_bloques_actuales = realloc(lista_bloques_actuales, (i + 1) * sizeof(uint32_t));
        lista_bloques_actuales[i] = bloque_libre;

        // Buscar el siguiente bloque libre
        bloque_libre = buscar_primer_bloque_libre_fat(archivo_fat);

        if (bloque_libre == -1) {
            // Manejar el caso en que no hay más bloques libres
            perror("No hay más bloques libres disponibles");
            break;
        }

        log_info(logger,"Acceso FAT - Entrada: %d", bloque_libre);

        bloque_asignar_desde = bloque_libre;  // Actualizar el bloque desde el cual asignar en la siguiente iteración
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

void desasignar_bloques_a_archivo(const char* path_fat, t_config* archivo_fcb, int tamanio_a_reducir, int tamanio_bloque) {
    int cantidad_bloques_a_liberar = tamanio_a_reducir / tamanio_bloque;
    int tamanio_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
    int cantidad_bloques_archivo = tamanio_archivo / tamanio_bloque;

    // Obtener la lista de bloques asignados actualmente
    uint32_t* lista_bloques_actuales = obtener_bloques_asignados(path_fat, archivo_fcb);

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
    FILE* archivo_fat = fopen(path_fat, "r+b");

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

    uint32_t bloque_actual = 0;
    fseek(archivo_fat, bloque_actual * sizeof(uint32_t), SEEK_SET);
    usleep(RETARDO_ACCESO_FAT * 1000);
    while (fread(&bloque_actual, sizeof(uint32_t), 1, archivo_fat) == 1) {

        if (bloque_actual == 0) {
            // Encontramos un bloque libre
            fclose(archivo_fat);
            return bloque_actual;
        }

        fseek(archivo_fat, bloque_actual * sizeof(uint32_t), SEEK_SET);
        usleep(RETARDO_ACCESO_FAT * 1000);
    }

    fclose(archivo_fat);

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


void crear_archivo_fat(const char* path_fat, int cantidad_bloques_fat) {
    // Abrir el archivo en modo de escritura binaria
    FILE* archivo_fat = fopen(path_fat, "wb");

    // Crear un array de uint32_t con el tamaño deseado
    uint32_t* entradas = calloc(cantidad_bloques_fat, sizeof(uint32_t)); // Inicializa con 0 automáticamente

    // Escribir el array al archivo
    usleep(RETARDO_ACCESO_FAT * 1000);
    fwrite(entradas, sizeof(uint32_t), cantidad_bloques_fat, archivo_fat);

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
void crear_archivo_bloques(char* path){

	int tamanio_archivo_bloques = CANT_BLOQUES_TOTAL*TAM_BLOQUE;
	FILE* archivo_bloques = fopen(path,"wb+");
	// deberia setear todas las entradas, tanto de swap como de fat en 0;
	fseek(archivo_bloques,0, SEEK_SET);
	for(int i = 0; i < tamanio_archivo_bloques; i++) {
		fwrite("0", sizeof(uint32_t), 1, archivo_bloques);
	}

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





