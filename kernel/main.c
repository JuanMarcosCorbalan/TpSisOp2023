#include "include/main.h"

int main(void)
{
	t_log* logger;
	t_config* config;
	logger = iniciar_logger();
	config = iniciar_config();
	char* valor;
	int fd_cpu = 0;
	int fd_filesystem = 0;
	int fd_memoria = 0;

	valor = config_get_string_value(config,"CLAVE");

	if(!conectar_modulos(logger, config, &fd_cpu, &fd_filesystem, &fd_memoria)){
		terminar_programa(fd_cpu, fd_filesystem, fd_memoria, logger, config);
		return EXIT_FAILURE;
	}

	enviar_mensaje(valor, fd_cpu);
	enviar_mensaje(valor, fd_filesystem);
	enviar_mensaje(valor, fd_memoria);

	paquete(fd_cpu);
	paquete(fd_filesystem);
	paquete(fd_memoria);

	terminar_programa(fd_cpu, fd_filesystem, fd_memoria, logger, config);

	return EXIT_SUCCESS;
}

t_log* iniciar_logger(void)
{
	t_log* nuevo_logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);

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

void terminar_programa(int conexion, int conexion2, int conexion3, t_log* logger, t_config* config)
{
	if(logger != NULL){
		log_destroy(logger);
	}

	if(config != NULL){
		config_destroy(config);
	}

	liberar_conexion(conexion);
	liberar_conexion(conexion2);
	liberar_conexion(conexion3);
}

bool conectar_modulos(t_log* logger, t_config* config, int* fd_cpu, int* fd_filesystem, int* fd_memoria){

	char* ip;
	char* puerto;
	char* puerto_filesystem;
	char* puerto_memoria;

	ip = config_get_string_value(config, "IP");
	puerto = config_get_string_value(config, "PUERTO");
	puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

	*fd_cpu = crear_conexion(logger, ip, puerto);
	*fd_filesystem = crear_conexion(logger, ip, puerto_filesystem);
	*fd_memoria = crear_conexion(logger, ip, puerto_memoria);

	return *fd_cpu != 0 && *fd_filesystem != 0 && *fd_memoria != 0;
}
