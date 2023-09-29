/*
 * configuracionKernel.h
 *
 *  Created on: Sep 26, 2023
 *      Author: utnso
 */

#ifndef CONFIGURACIONKERNEL_H_
#define CONFIGURACIONKERNEL_H_

#include <commons/config.h>
#include <pthread.h>
#include <commons/collections/queue.h>
#include "../../mappaLib/include/utils.h"

// STRUCTS

typedef struct kernel_config{

	char* IP_MEMORIA;
	char* PUERTO_MEMORIA;
	char* IP_FILESYSTEM;
	char* PUERTO_FILESYSTEM;
	char* IP_CPU;
	char* PUERTO_CPU_DISPATCH;
	char* PUERTO_CPU_INTERRUPT;
	char* ALGORITMO_PLANIFICACION;
	char* QUANTUM;
	char** RECURSOS;
	char* INSTANCIAS_RECURSOS;
	char* GRADO_MULTIPROGRAMACION_INI;

} t_kernel_config;

t_pcb* proximoAEjecutar = NULL;
pthread_mutex_t mutex_ready_list;
t_log* logger;

// CONFIG

extern t_kernel_config lecturaConfig;

// FUNCIONES

t_kernel_config leerKernelConfig(t_config* config);


#endif /* CONFIGURACIONKERNEL_H_ */
