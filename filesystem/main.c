#include "include/main.h"

t_log* logger;

int main(void) {
	logger = log_create("filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_DEBUG);

	t_config* config;
	t_list* tabla_fat;
	char* ip;
	char* puerto_escucha;
	char* puerto_memoria;
	int fd_memoria = 0;
	config = iniciar_config();
	ip = config_get_string_value(config, "IP");
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	int cant_bloques_total = atoi(config_get_string_value(config, "CANT_BLOQUES_TOTAL"));
	int cant_bloques_swap = atoi(config_get_string_value(config, "CANT_BLOQUES_SWAP"));
	int tamanio_fat = cant_bloques_total - cant_bloques_swap;
	int tam_bloque = atoi(config_get_string_value(config, "TAM_BLOQUE"));
	int cant_bloques = tamanio_fat / tam_bloque;

//	fd_memoria = crear_conexion(logger, ip, puerto_memoria);
//	enviar_mensaje("Hola, soy un File System!", fd_memoria);
//
//	int server_fd = iniciar_servidor(puerto_escucha);
//	log_info(logger, "FILESYSTEM LISTO...");
//	while(experar_clientes(logger, server_fd));
//	log_info(logger, "KERNEL CONECTADO A FS");


	tabla_fat = inicializar_fat(tamanio_fat);
	crear_archivo("hola",tabla_fat);
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

//void solicitud_peticion(){
//	// como hago para tener multiples instancias? SERIA UNA POR SOCKET
//	pthread_t hilo_peticion;
//	pthread_create(&hilo_peticion, NULL, (void*) recibir_peticion_cpu, NULL);
//	pthread_detach(hilo_peticion);
//}

void recibir_peticion_cpu(){
	// aca va a llegar peticiones tales como abrir archivo, truncar, leer o escribir. ¿que hago con crear y cerrar?
}


//t_list* swap{
//
//}

uint32_t* inicializar_fat(int cantidad_bloques_fat){ // se inicializan todas las entradas de la tabla fat
//	t_list* fat = list_create();
//	fat->elements_count = tamanio_fat;
//	for (int i = 0; i < list_size(fat); i++){
//		uint32_t* entrada = list_get(fat, i);
//		entrada = 0;
//	return fat;

//	}

	uint32_t* tabla_fat[cantidad_bloques_fat];
	for (int i = 0; i<cantidad_bloques_fat; i++) {
		tabla_fat[i] = 0;
	}
	return tabla_fat;
}

FILE* cargar_fat_en_archivo(uint32_t* tabla_fat, int cantidad_bloques_fat, uint32_t tamanio_swap){
	FILE *archivo_bloques = fopen("./ArchivoDeBloques", "wb+");
	fseek(archivo_bloques, tamanio_swap, SEEK_SET); // busca hasta el final de la swap

	for(int i = 0; i < cantidad_bloques_fat; i++){
		int bloque_actual = tabla_fat[i];
		fwrite(bloque_actual,sizeof(tabla_fat), 1 , archivo_bloques);
	}
	return archivo_bloques;
}

uint32_t* obtener_bloque_siguiente(int bloque_actual, uint32_t* tabla_fat){
	uint32_t* bloque_siguiente = 0;
	bloque_siguiente = tabla_fat[bloque_actual];
	return bloque_siguiente;
}

t_config* crear_archivo_fcb(t_fcb* fcb){
	//segun el enunciado conviene hacerlo como si fuera un config
	char tamanio_archivo[] = "VACIO";
	char bloque_inicial[] = "NO_ASIGNADO";
	sprintf(tamanio_archivo, "%d", fcb->tamanio_archivo);
	sprintf(bloque_inicial, "%d", fcb->bloque_inicial);
//	bloque_inicial = itoa(fcb->bloque_inicial);
	t_config* archivo_fcb;
	archivo_fcb = config_create(("./%s",fcb->nombre_archivo));
	config_set_value(archivo_fcb,"NOMBRE_ARCHIVO",fcb->nombre_archivo);
	config_set_value(archivo_fcb,"TAMANIO_ARCHIVO",tamanio_archivo);
	config_set_value(archivo_fcb,"BLOQUE_INICIAL",bloque_inicial);
	return archivo_fcb;
}

void crear_fcb(char* nombre, int tamanio, int bloque){
	t_fcb* nuevo_fcb;
	nuevo_fcb->nombre_archivo = nombre;
	nuevo_fcb->tamanio_archivo = tamanio;
	nuevo_fcb->bloque_inicial = bloque;
	crear_archivo_fcb(nuevo_fcb);
}

void crear_archivo(char* nombreArchivoNuevo, t_list* fat){
	crear_fcb(nombreArchivoNuevo, 0, 0);//deberia pasarle como argumentos el tamaño inicial 0 y bloque inicial null

}

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

