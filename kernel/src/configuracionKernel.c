/*
 * configuracionKernel.c
 *
 *  Created on: Sep 26, 2023
 *      Author: utnso
 */

#include "../include/configuracionKernel.h"
#include <commons/log.h>
#include <commons/collections/queue.h>

t_kernel_config lecturaConfig;

// LOGGERS

t_log* logger;
t_log* my_logger;

// LISTAS Y COLAS

t_list* ready_list;

// VARIABLES COMPARTIDAS - GLOBALES

t_pcb* proximoAEjecutar = NULL;

// SEMAFOROS

pthread_mutex_t mutex_ready_list;

// SOCKETS

int socket_cpu;
int socket_filesystem;
int socket_memoria;


// FUNCIONES

t_kernel_config leerKernelConfig(t_config* config){

	t_kernel_config lecturaConfig;

	lecturaConfig.IP_MEMORIA = strdup(config_get_string_value(config, "IP_MEMORIA"));
	lecturaConfig.PUERTO_MEMORIA = strdup(config_get_string_value(config, "PUERTO_MEMORIA"));
	lecturaConfig.IP_FILESYSTEM = strdup(config_get_string_value(config, "IP_FILESYSTEM"));
	lecturaConfig.PUERTO_FILESYSTEM = strdup(config_get_string_value(config, "PUERTO_FILESYSTEM"));
	lecturaConfig.IP_CPU = strdup(config_get_string_value(config, "IP_CPU"));
	lecturaConfig.PUERTO_CPU_DISPATCH = strdup(config_get_string_value(config, "PUERTO_CPU_DISPATCH"));
	lecturaConfig.PUERTO_CPU_INTERRUPT = strdup(config_get_string_value(config, "PUERTO_CPU_INTERRUPT"));
	lecturaConfig.ALGORITMO_PLANIFICACION = strdup(config_get_string_value(config, "ALGORITMO_PLANIFICACION"));
	lecturaConfig.QUANTUM = strdup(config_get_double_value(config, "QUANTUM"));
	lecturaConfig.GRADO_MULTIPROGRAMACION_INI = strdup(config_get_int_value(config, "GRADO_MULTIPROGRAMACION_INI"));

	char* recursos = strdup(config_get_string_value(config, "RECURSOS")); //"[elem1, elem2, ...]"
	char* recursos_sin_espacio = string_replace(recursos, " " , ""); //"[elem1,elem2,...]"
	free(recursos);
	lecturaConfig.RECURSOS = string_get_string_as_array(recursos_sin_espacio); //["elem1", "elem2", ...]
	free(recursos_sin_espacio);

	char* instancias_recursos = strdup(config_get_string_value(config, "INSTANCIAS_RECURSOS"));
	char* instancias_recursos_sin_espacio = string_replace(instancias_recursos, " " , "");
	free(instancias_recursos);
	lecturaConfig.INSTANCIAS_RECURSOS = string_get_string_as_array(instancias_recursos_sin_espacio);
	free(instancias_recursos_sin_espacio);

	return lecturaConfig;

}
