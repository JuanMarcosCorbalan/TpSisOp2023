#include "include/main.h"

t_log* logger;
t_config* config;

int main(void) {
	logger = log_create("filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_DEBUG);

	char* ip;
	char* puerto_escucha;
	char* puerto_memoria;
	int fd_memoria = 0;
	int fd_cliente = 0;
	FILE* archivo_bloques;
	FILE* archivo_fat;

	char* path_fat = config_get_string_value(config, "PATH_FAT");
	char* path_fcb = config_get_string_value(config, "PATH_FCB");
	char* path_bloques = config_get_string_value(config, "PATH_BLOQUES");

	config = iniciar_config();
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


	while(experar_clientes(logger, server_fd));
	log_info(logger, "KERNEL CONECTADO A FS");


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

void procesar_conexion(){
	// aca va a llegar peticiones tales como abrir archivo, truncar, leer o escribir. ¿que hago con crear y cerrar?
	op_code codigo_operacion;

	while (socket_cliente != -1) {

	}
}

void realizar_operacion(t_operacion* operacion){
	codigo_operacion_fs operacion_fs = operacion -> cod_op;
	switch(operacion_fs){
	case ABRIR_ARCHIVO_FS:
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
		break;
	case LEER_ARCHIVO_FS:
		break;
	case ESCRIBIR_ARCHIVO_FS:
		break;
	case INICIAR_PROCESO_FS:
		break;
	case FINALIZAR_PROCESO_FS:
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

void procesar_F_READ(char* nombre_archivo, int direccion_fisica, int puntero, char* path_fat, int fd_memoria){
	// establecer el puntero a partir de la direccion_fisica
	// creo que el puntero pasaria por parametro, hay que ubicarlo en el primer byte del bloque a leer
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
	    fread(datos_leidos_bloque, tamanio_buffer, 1, archivo_bloques);
	}
	else{
	    puts("Something wrong reading from File.\n");
	}

	// o me conviene dejarlo abierto?
	fclose(archivo_bloques);


	return datos_leidos_bloque;
}

void escribir_datos_bloque_archivo(t_config* archivo_fcb, int bloque_a_escribir, char* buffer_escritura){
	int tamanio_bloque = config_get_int_value(archivo_fcb, "TAMANIO_BLOQUE");
	char* datos_escritos_bloque = malloc(tamanio_bloque);
	FILE* archivo_bloques = fopen("./ArchivoDeBloques", "wb+");
	int tamanio_buffer = strlen(buffer_escritura) + 1;

	if (archivo_bloques){
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

void escribir_archivo(char* nombre_archivo, int direccion_fisica, int puntero, char* buffer_a_escribir, int tamanio_a_escribir,const char* path_fat){
	// primero se recibe en el buffer lo que se va a escribir (y oh sorpresa, despues lo hago :))
	//cuando se escribe un archivo, primero hay que truncarlo, modificar el fcb con el nuevo tamaño y escribir en los bloques

	char* path = convertir_path_fcb(nombre_archivo);
	char* bufferEscrito;
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
    uint32_t bloque_libre = buscar_primer_bloque_libre(path_fat);

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
    FILE* archivo_fat = fopen(path_fat, "r+b");

    if (archivo_fat == NULL) {
        perror("Error al abrir el archivo FAT para lectura y escritura binaria");
        free(lista_bloques_actuales);
        return;
    }

    // Actualizar la tabla FAT con la secuencia de bloques asignados
    for (int i = 0; i < cantidad_bloques_solicitados; i++) {
        fseek(archivo_fat, bloque_asignar_desde * sizeof(uint32_t), SEEK_SET);
        fwrite(&bloque_libre, sizeof(uint32_t), 1, archivo_fat);

        // Agregar el bloque a la lista de bloques asignados del archivo
        lista_bloques_actuales = realloc(lista_bloques_actuales, (i + 1) * sizeof(uint32_t));
        lista_bloques_actuales[i] = bloque_libre;

        // Buscar el siguiente bloque libre
        bloque_libre = buscar_primer_bloque_libre(path_fat);

        if (bloque_libre == -1) {
            // Manejar el caso en que no hay más bloques libres
            perror("No hay más bloques libres disponibles");
            break;
        }

        log_info(logger,"Acceso FAT - Entrada: %s");

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

uint32_t buscar_primer_bloque_libre(const char* path_fat) {
    FILE* archivo_fat = fopen(path_fat, "rb");

    if (archivo_fat == NULL) {
        perror("Error al abrir el archivo FAT para lectura binaria");
        return -1; // Otra forma de indicar un error
    }

    uint32_t bloque_actual = 0;
    fseek(archivo_fat, bloque_actual * sizeof(uint32_t), SEEK_SET);

    while (fread(&bloque_actual, sizeof(uint32_t), 1, archivo_fat) == 1) {
        if (bloque_actual == 0) {
            // Encontramos un bloque libre
            fclose(archivo_fat);
            return bloque_actual;
        }

        fseek(archivo_fat, bloque_actual * sizeof(uint32_t), SEEK_SET);
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
	FILE* archivo_bloques = fopen(path,"wb+");

};

