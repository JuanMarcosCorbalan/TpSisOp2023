/*
 * planificadorCorto.h
 *
 *  Created on: Sep 26, 2023
 *      Author: utnso
 */

#ifndef PLANIFICADORCORTO-H
#define PLANIFICADORCORTO-H

#include <utils.h>
#include <pthread.h>

typedef struct{
	int tiempo;
	t_pcb* pcb;
};

void planificadorCorto();
t_pcb* obtenerProximoAEjecutar();

#endif

