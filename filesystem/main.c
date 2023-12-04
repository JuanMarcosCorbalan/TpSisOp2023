#include "include/main.h"

t_log* logger;

int main(void) {
	logger = log_create("filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_DEBUG);


	t_config* config;
	t_bloque* tabla_fat;
	char* ip;
	char* puerto_escucha;
	char* puerto_memoria;
	int fd_memoria = 0;
	FILE* archivo_bloques;
	config = iniciar_config();
	ip = config_get_string_value(config, "IP");
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	int cant_bloques_total = atoi(config_get_string_value(config, "CANT_BLOQUES_TOTAL"));
	int cant_bloques_swap = atoi(config_get_string_value(config, "CANT_BLOQUES_SWAP"));
	int cant_bloques_fat = cant_bloques_total - cant_bloques_swap;
	int tam_bloque = atoi(config_get_string_value(config, "TAM_BLOQUE"));
	int tamanio_fat = cant_bloques_fat * tam_bloque;
//	int tamanio_swap = cant_bloques_swap * tam_bloque; NO LO USO
//	int tamanio_archivo_bloques = tamanio_swap + tamanio_fat;


	fd_memoria = crear_conexion(logger, ip, puerto_memoria);
	enviar_mensaje("Hola, soy un File System!", fd_memoria);


	int server_fd = iniciar_servidor(puerto_escucha);
	log_info(logger, "FILESYSTEM LISTO...");


	while(experar_clientes(logger, server_fd));
	log_info(logger, "KERNEL CONECTADO A FS");


//	while(manejar_peticiones());


	// esto deberia ir en manejar solicitudes? o antes?
	tabla_fat = inicializar_fat(tamanio_fat);
//	archivo_bloques = cargar_fat_en_archivo(tabla_fat, cant_bloques, tamanio_swap);
	t_config* archivo_fcb = crear_archivo_fcb("hola");

	archivo_bloques = fopen("./ArchivoDeBloques", "wb+");
	if (archivo_bloques == NULL) {
		crear_archivo_bloques("./ArchivoDeBloques");
		log_info(logger, "No se encontro el archivo de bloques, se creo uno nuevo");
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

//void manejar_peticiones(){
//	// como hago para tener multiples instancias? SERIA UNA POR SOCKET
//	pthread_t hilo_peticion;
//	pthread_create(&hilo_peticion, NULL, (void*) procesar_peticion, NULL);
//	pthread_detach(hilo_peticion);
//
//
//}

void procesar_peticion(){
	// aca va a llegar peticiones tales como abrir archivo, truncar, leer o escribir. ¿que hago con crear y cerrar?

}

void crear_archivo(){ // ya la tengo como crear_archivo_fcb (ya que es lo unico que hace)

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
		free(path);
		return 0;
	} else {
		log_info(logger, "Abrir Archivo: %s", nombre_archivo);
		t_config* archivo_fcb = config_create(path);
		int tamanio_archivo = config_get_int_value(archivo_fcb,"TAMANIO_ARCHIVO");
		return tamanio_archivo;
	}
}

void leer_archivo(char* nombre_archivo, int direccion_fisica){
	// establecer el puntero a partir de la direccion_fisica
	// se establece en el puntero



	// se guarda la info leida y se envia a memoria empaquetada
	// memoria hace un handshake
	// luego se envia un mensajito a kernel diciendo que el traspaso de info fue exitoso
}

//void escribir_archivo(char* nombre_archivo, int direccion_fisica){
//	//cuando se escribe un archivo, primero hay que truncarlo, modificar el fcb con el nuevo tamaño y escribir en los bloques
//	char* path = convertir_path_fcb(nombre_archivo);
//	t_config* archivo_fcb = config_create(path);
//	int tamanio_archivo = atoi(config_get_string_value(archivo_fcb, "TAMANIO_ARCHIVO"));
//	if(){
//
//	}
//}

// todo tengo que ver como obtiene el tamanio a cambiar
void truncar_archivo(t_config* config, t_config* archivo_fcb, int tamanio_a_cambiar, t_bloque* tabla_fat) { // SUPUSE QUE EL TAMAÑO PUEDE SER NEGATIVO SI SE REDUZCO EL ARCHIVO
//	int tamanio_actual_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO"); NO LA USO
	char* nombre_archivo = config_get_string_value(archivo_fcb, "NOMBRE_ARCHIVO");
	if(tamanio_a_cambiar > 0){
		agrandar_archivo(config, archivo_fcb, tamanio_a_cambiar, tabla_fat);
		log_info(logger, "Truncar archivo: %s - Tamanio: %d", nombre_archivo, tamanio_a_cambiar);

	} else  {
		if (tamanio_a_cambiar < 0){
		reducir_archivo(config, archivo_fcb, tamanio_a_cambiar, tabla_fat);
		log_info(logger, "Truncar archivo: %s - Tamanio: %d", nombre_archivo, tamanio_a_cambiar);
		} else {
			log_info(logger, "No es necesario truncar el archivo %s", nombre_archivo);
		}
	}
}

void agrandar_archivo(t_config* config, t_config* archivo_fcb, int tamanio_a_agregar, t_bloque* tabla_fat){

	// seteo el nuevo tamanio en el fcb
//	int tamanio_actual_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO"); NO LA USO
//	int nuevo_tamanio = tamanio_actual_archivo + tamanio_a_agregar;	NO LA USO
	int tamanio_bloque = config_get_int_value(config, "TAMANIO_BLOQUE");
//	config_set_int_value(archivo_fcb,"TAMANIO_ARCHIVO", nuevo_tamanio);
	// tengo que ver como hago para pasar de int a string

	// ahora tendria que reservar o asignar bloques
	asignar_bloques_archivo(archivo_fcb, config, tamanio_a_agregar, tabla_fat, tamanio_bloque);
}

void reducir_archivo(t_config* config, t_config* archivo_fcb, int tamanio_a_reducir, t_bloque* tabla_fat){
	// seteo el nuevo tamanio en el fcb
//		int tamanio_actual_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO"); NO LA USO
//		int nuevo_tamanio = tamanio_actual_archivo - tamanio_a_reducir; NO LA USO
		int tamanio_bloque = config_get_int_value(config, "TAMANIO_BLOQUE");
//		config_set_int_value(archivo_fcb,"TAMANIO_ARCHIVO", nuevo_tamanio);
		// tengo que ver como hago para pasar de int a string
		// ahora tendria que reservar o asignar bloques
		desasignar_bloques_archivo(archivo_fcb, config, tamanio_a_reducir, tabla_fat, tamanio_bloque);
}

void asignar_bloques_archivo(t_config* archivo_fcb, t_config* config, int tamanio_a_agregar, t_bloque* tabla_fat, int tamanio_bloque){
	int cantidad_bloques_solicitados = tamanio_a_agregar / tamanio_bloque; // me tendria que dar cuantos bloques necesita, se asignan obviamente bloques enteros
	int bloque_asignado = 0;
	int ultimo_bloque_asignado = 0;
	if (archivo_sin_bloques(archivo_fcb)) // si el archivo no tiene ningun bloque, se va a buscar para asignar el primero y cambiarlo en el fcb
	{
		int primer_bloque_archivo = buscar_bloque_libre_fat(tabla_fat);
//		config_set_int_value(archivo_fcb, "PRIMER_BLOQUE", primer_bloque_archivo);
		// tengo que ver como hago para pasar de int a string
		tabla_fat[primer_bloque_archivo].codigo_bloque = 2;
		cantidad_bloques_solicitados--;
	}
	while (cantidad_bloques_solicitados != 0) { // se van a ir asignando bloques uno por uno hasta que ya no queden bloques solicitados
		bloque_asignado = buscar_bloque_libre_fat(tabla_fat);
		ultimo_bloque_asignado = ultimo_bloque_archivo_fat(archivo_fcb, config, tabla_fat);
		tabla_fat[ultimo_bloque_asignado].codigo_bloque = 1; //OCUPADO
		tabla_fat[ultimo_bloque_asignado].numero_bloque_siguiente= bloque_asignado;
		tabla_fat[bloque_asignado].codigo_bloque = 2; // BLOQUE FINAL
	}
}

void desasignar_bloques_archivo(t_config* archivo_fcb, t_config* config, int tamanio_a_reducir, t_bloque* tabla_fat, int tamanio_bloque){
	int cantidad_bloques_a_reducir = tamanio_a_reducir / tamanio_bloque; // considero asi porq supuestamente siempre vamos a trabajar con bloques enteros
//	int bloque_asignado = 0;
	int ultimo_bloque_asignado = ultimo_bloque_archivo_fat(archivo_fcb, config, tabla_fat);
	int i;
	while (cantidad_bloques_a_reducir != 0) {
		tabla_fat[ultimo_bloque_asignado].codigo_bloque = 0; //seteo el ultimo bloque como libre
		if (tabla_fat[i].numero_bloque_siguiente != ultimo_bloque_asignado) i++; // busco el anterior al ultimo bloque
		tabla_fat[i].codigo_bloque = 2; // ahora ese es el ultimo bloque
		ultimo_bloque_asignado = ultimo_bloque_archivo_fat(archivo_fcb, config, tabla_fat); // vuelvo a buscar el ultimo bloque
		cantidad_bloques_a_reducir--;
	}
}

bool archivo_sin_bloques(t_config* archivo_fcb){
	int primer_bloque = config_get_int_value(archivo_fcb, "PRIMER_BLOQUE");
	if(primer_bloque == 0){
		return true;
	} else return false;
}

// ultimo bloque archivo fat hace referencia al ultimo bloque que corresponde a un archivo en la fat, osea busca por la cadena de ESE ARCHIVO, no es el ultimo bloque de la fat
int ultimo_bloque_archivo_fat(t_config* archivo_fcb, t_config* config, t_bloque* tabla_fat){
	int ultimo_bloque_asignado = 0;
	t_bloque* aux_tabla = tabla_fat;
//	int tamanio_actual_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
//	int tamanio_bloque = config_get_int_value(config, "TAMANIO_BLOQUE");
//	int cantidad_bloques_asignados = tamanio_actual_archivo / tamanio_bloque;
	int primer_bloque = config_get_int_value(archivo_fcb, "PRIMER_BLOQUE");
	int bloque_siguiente = 0;
	int i = primer_bloque;
	while(tabla_fat[i].codigo_bloque != 2) {
		bloque_siguiente = aux_tabla[i].numero_bloque_siguiente;
		i = bloque_siguiente;
	}
	ultimo_bloque_asignado = i;

	return ultimo_bloque_asignado;
}

//t_list* swap{
//
//}

t_bloque* inicializar_fat(int cantidad_bloques_fat){ // se inicializan todas las entradas de la tabla fat
	t_bloque* tabla_fat = malloc(cantidad_bloques_fat * sizeof(t_bloque));
	if (tabla_fat == NULL) {
		// Manejar error de asignación de memoria
		fprintf(stderr, "Error al asignar memoria para la tabla FAT\n");
		return NULL;
	}

	tabla_fat[0].codigo_bloque = 2; // el primero es reservado
	for (int i = 1; i<cantidad_bloques_fat; i++) {
		tabla_fat[i].codigo_bloque = 0; // todos estan libres
	}
	return tabla_fat;
}

int buscar_bloque_libre_fat(t_bloque* tabla_fat){
	int i = 1;
	int numero_bloque_libre = 0;

	while(tabla_fat[i].codigo_bloque != 0) i++;
	numero_bloque_libre = i;

	return numero_bloque_libre;
	// no cierro el archivo para que quede abierto hasta que termine de ejecutar el fs
}



uint32_t obtener_bloque_siguiente(int bloque_actual, int* tabla_fat){
	uint32_t bloque_siguiente = 0;
	bloque_siguiente = tabla_fat[bloque_actual];
	return bloque_siguiente;
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

}


// A ESTA FUNCION POR AHORA NO LE DOY BOLA PERO NO SE COMO HACER PARA REPRESENTAR UN BLOQUE VACIO
// TODO NO HACE FALTA PORQ NO SE VA A LLEGAR AL CASO DE LLENAR EL FS PERO CREO QUE NO ESTARIA DE MAS HACERLA
//bool archivo_entra_en_fat(t_list* fat, int tamanio_fat, int tam_bloque, t_fcb* fcb_archivo) {
//	int cantidad_bloques_libres = 0;
//	for (int i = 0; i < list_size(fat); i++){
//		int bloque_actual = list_get(fat, i);
//		if (bloque_actual == 0){
//			cantidad_bloques_libres++;
//		}
//	}
//	if(cantidad_bloques_libres > (fcb_archivo->tamanio_archivo / tam_bloque)) {
//		return true;
//	} else return false;
//}

// creo que voy a hacer la estructura de la fat yo
// va a ser un array y cada elemento va a ser un int al siguiente bloque (tambien van a arrancar desde el 0, pero el bloque 0 no lo puedo asignar a nadie)



// para almacenar, se guarda to/do en un archivo binario general, que tiene swap y fat (dos particiones)

