/*
 * planificadorCorto.h
 *
 *  Created on: Sep 26, 2023
 *      Author: utnso
 */

#ifndef PLANIFICADORCORTO_H_
#define PLANIFICADORCORTO_H_

#include <pthread.h>
#include "configuracionKernel.h"
#include "comunicaciones_kernel.h"
#include "../../mappaLib/include/utils.h"
#include "../../mappaLib/include/protocolo.h"

//typedef struct{
//	int tiempo;
//	t_pcb* pcb;
//} ;

void planificadorCorto();
t_pcb* obtenerProximoAEjecutar();

#endif

