/*
 * configuracionKernel.h
 *
 *  Created on: Sep 26, 2023
 *      Author: utnso
 */

#ifndef CONFIGURACIONKERNEL_H_
#define CONFIGURACIONKERNEL_H_

#include <utils.h>
#include <commons/config.h>
#include <pthread.h>
#include <commons/collections/queue.h>

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
	double QUANTUM;
	char** RECURSOS;
	char* INSTANCIAS_RECURSOS;
	int GRADO_MULTIPROGRAMACION_INI;

} t_kernel_config;


// CONFIG

extern t_kernel_config lecturaConfig;

// FUNCIONES

t_kernel_config leerKernelConfig(t_config* config);



#endif /* CONFIGURACIONKERNEL_H_ */
