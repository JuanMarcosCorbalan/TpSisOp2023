#include "include/main.h"

int main(void)
{
	int conexion;
	int conexionFilesystem;
	char* ip;
	char* puerto;
	char* puertoFilesystem;
	char* valor;

	t_log* logger;
	t_config* config;

	logger = iniciar_logger();

	config = iniciar_config();

	ip = config_get_string_value(config, "IP");
	puerto = config_get_string_value(config, "PUERTO");
	valor = config_get_string_value(config,"CLAVE");
	puertoFilesystem = config_get_string_value(config, "PUERTOFILESYSTEM");

	log_info(logger, "Se leyo la IP: %s , el PUERTO: %s , el PUERTOFILESYSTEM: %s y el VALOR: %s\n", ip, puerto, puertoFilesystem, valor);

	// Creamos una conexión hacia CPU
	conexion = crear_conexion(ip, puerto);
	// Creamos una conexión hacia FILESYSTEM
	conexionFilesystem = crear_conexion(ip, puertoFilesystem);

	//Envio de mensajes
	enviar_mensaje(valor, conexion);
	enviar_mensaje(valor, conexionFilesystem);

	//Envio de paquetes
	paquete(conexion);
	paquete(conexionFilesystem);

	terminar_programa(conexion, logger, config);
	terminar_programa(conexionFilesystem, logger, config);
}

t_log* iniciar_logger(void)
{
	t_log* nuevo_logger = log_create("tp0.log", "pruebaLogger1", 1, LOG_LEVEL_INFO);

	if(nuevo_logger == NULL){
		printf("No se pudo leer el logger\n");
		exit(1);
	}

	return nuevo_logger;
}

t_config* iniciar_config(void)
{
	t_config* nuevo_config = config_create("./kernel.config");

	if(nuevo_config == NULL){
		printf("No se pudo leer el config\n");
		exit(2);
	}

	return nuevo_config;
}

void leer_consola(t_log* logger)
{
	char* leido;

	leido = readline("> ");
	while(strcmp(leido, "")){
		log_info(logger, "Se leyo la linea de consola: %s", leido);
		leido = readline("> ");
	}

	free(leido);
}

void paquete(int conexion)
{
	char* leido;
	t_paquete* paquete = crear_paquete();

	leido = readline("> ");
	while(strcmp(leido, "")){
		agregar_a_paquete(paquete, leido, strlen(leido) +1);
		leido = readline("> ");
	}

	enviar_paquete(paquete, conexion);

	free(leido);
	eliminar_paquete(paquete);
}

void terminar_programa(int conexion, t_log* logger, t_config* config)
{
	if(logger != NULL){
		log_destroy(logger);
	}

	if(config != NULL){
		config_destroy(config);
	}

	liberar_conexion(conexion);
}
