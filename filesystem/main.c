#include "include/main.h"

t_log* logger;

int main(void) {
	logger = log_create("filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_DEBUG);

	t_config* config;
	char* ip;
	char* puerto_escucha;
	char* puerto_memoria;
	int fd_memoria = 0;
	config = iniciar_config();
	ip = config_get_string_value(config, "IP");
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	int cant_bloques_total = config_get_string_value(config, "CANT_BLOQUES_TOTAL");
	int cant_bloques_swap = config_get_string_value(config, "CANT_BLOQUES_SWAP")
	int tamanio_fat = cant_bloques_total - cant_bloques_swap;
	fd_memoria = crear_conexion(logger, ip, puerto_memoria);
	enviar_mensaje("Hola, soy un File System!", fd_memoria);

	int server_fd = iniciar_servidor(puerto_escucha);
	log_info(logger, "FILESYSTEM LISTO...");
	while(experar_clientes(logger, server_fd));
	log_info(logger, "KERNEL CONECTADO A FS");


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

void solicitud_peticion(){
	// como hago para tener multiples instancias? SERIA UNA POR SOCKET
	pthread_t hilo_peticion;
	pthread_create(&hilo_peticion, NULL, (void*) recibir_peticion_cpu, NULL);
	pthread_detach(hilo_peticion);
}

void recibir_peticion_cpu(){
	// aca va a llegar peticiones tales como abrir archivo, truncar, leer o escribir. ¿que hago con crear y cerrar?
}



//t_list* swap{
//
//}
t_list* inicializar_fat(int tamanio_fat){
	t_list* fat = list_create();
	fat->elements_count = tamanio_fat;
	for (int i = 0; i < list_size(fat); i++){
		uint32_t* entrada = list_get(fat, i);
		entrada = 0;
	}
	return fat;
}

t_fcb* crear_fcb(char* nombre, int tamanio, int bloque){
	t_fcb* nuevo_fcb;
	nuevo_fcb->nombre_archivo = nombre;
	nuevo_fcb->tamanio_archivo = tamanio;
	nuevo_fcb->bloque_inicial = bloque;
	return nuevo_fcb;
}

void crear_archivo(){
	crear_fcb();//deberia pasarle como argumentos el tamaño inicial 0 y bloque inicial null

}

