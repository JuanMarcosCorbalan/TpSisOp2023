#include "include/main.h"

t_log* logger;

int main(void) {
	logger = log_create("filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_DEBUG);


	t_config* config;
	uint32_t* tabla_fat;
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
	int tamanio_swap = cant_bloques_swap * tam_bloque;
	int tamanio_archivo_bloques = tamanio_swap + tamanio_fat;
	fd_memoria = crear_conexion(logger, ip, puerto_memoria);
	enviar_mensaje("Hola, soy un File System!", fd_memoria);

	int server_fd = iniciar_servidor(puerto_escucha);
	log_info(logger, "FILESYSTEM LISTO...");
	while(experar_clientes(logger, server_fd));
	log_info(logger, "KERNEL CONECTADO A FS");


	while(manejar_peticiones());
	// esto deberia ir en manejar solicitudes? o antes?
	tabla_fat = inicializar_fat(tamanio_fat);
//	archivo_bloques = cargar_fat_en_archivo(tabla_fat, cant_bloques, tamanio_swap);
	t_config* archivo_fcb = crear_archivo_fcb("hola");

	archivo_bloques = fopen("./ArchivoDeBloques", "wb+");
	if (archivo_bloques == NULL) {
		crear_archivo_bloques(tamanio_archivo_bloques);
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

void manejar_peticiones(){
	// como hago para tener multiples instancias? SERIA UNA POR SOCKET
	pthread_t hilo_peticion;
	pthread_create(&hilo_peticion, NULL, (void*) recibir_peticion_cpu, NULL);
	pthread_detach(hilo_peticion);
}

void recibir_peticion_cpu(){
	// aca va a llegar peticiones tales como abrir archivo, truncar, leer o escribir. ¿que hago con crear y cerrar?
}

void crear_archivo(){

}
void abrir_archivo(){

}

void leer_archivo(){

}

void escribir_archivo(char* path){
	//cuando se escribe un archivo, primero hay que truncarlo, modificar el fcb con el nuevo tamaño y escribir en los bloques

	if ();
}

void truncar_archivo(char* path, int tamanio_a_cambiar) {

}

//t_list* swap{
//
//}

uint32_t* inicializar_fat(int cantidad_bloques_fat){ // se inicializan todas las entradas de la tabla fat
	uint32_t* tabla_fat = malloc(cantidad_bloques_fat * sizeof(uint32_t));
	if (tabla_fat == NULL) {
		// Manejar error de asignación de memoria
		fprintf(stderr, "Error al asignar memoria para la tabla FAT\n");
		return NULL;
	}
	for (int i = 0; i<cantidad_bloques_fat; i++) {
		tabla_fat[i] = 0;
	}
	return tabla_fat;
}

// esto esta mal, la fat no va en el archivo
// lo que va en el archivo son los datos
// la fat la voy a tratar como una estructura propia del fs para simplemente manejar los bloques
FILE* cargar_fat_en_archivo(uint32_t* tabla_fat, int cantidad_bloques_fat, uint32_t tamanio_swap){
	FILE *archivo_bloques = fopen("./ArchivoDeBloques", "wb+");
	fseek(archivo_bloques, tamanio_swap, SEEK_SET); // busca hasta el final de la swap

	for(int i = 0; i < cantidad_bloques_fat; i++){
		int bloque_actual = tabla_fat[i];
		fwrite(&bloque_actual,sizeof(tabla_fat), 1 , archivo_bloques);
	}
	return archivo_bloques;
	// no cierro el archivo para que quede abierto hasta que termine de ejecutar el fs
}



uint32_t obtener_bloque_siguiente(int bloque_actual, uint32_t* tabla_fat){
	uint32_t bloque_siguiente = 0;
	bloque_siguiente = tabla_fat[bloque_actual];
	return bloque_siguiente;
}

t_config* crear_archivo_fcb(char* nombre_archivo){
	//segun el enunciado conviene hacerlo como si fuera un config
	char* path = malloc(strlen("./fcbs/") + strlen(nombre_archivo) + strlen(".fcb") + 1); // +1 para el carácter nulo al final
	char* aux_nombre = malloc(strlen(nombre_archivo) + 1);
	strcpy(path, "./fcbs/");
	strcat(path, nombre_archivo);
	strcat(path, ".fcb");
	strcpy(aux_nombre, nombre_archivo);


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
void crear_archivo_bloques(int tamanio_archivo_bloques, char* path){

}

//void crear_archivo_en_fat(char* nombreArchivoNuevo){
//	crear_fcb(nombreArchivoNuevo, 0, 0);//deberia pasarle como argumentos el tamaño inicial 0 y bloque inicial null
//
//}

//void inicializar_fcb(t_fcb* fcb){
////	fcb->nombre_archivo =  malloc(strlen(fcb->nombre_archivo)+1);
//	fcb->tamanio_archivo = 0;
//	fcb->bloque_inicial = 0;
//
//}

// A ESTA FUNCION POR AHORA NO LE DOY BOLA PERO NO SE COMO HACER PARA REPRESENTAR UN BLOQUE VACIO
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



// para almacenar, se guarda todo en un archivo binario general, que tiene swap y fat (dos particiones)

