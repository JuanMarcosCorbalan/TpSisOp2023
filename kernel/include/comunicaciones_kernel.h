/*
 * comunicaciones_kernel.h
 *
 *  Created on: Sep 27, 2023
 *      Author: utnso
 */

#ifndef INCLUDE_COMUNICACIONES_KERNEL_H_
#define INCLUDE_COMUNICACIONES_KERNEL_H_

#include <commons/string.h>
#include <commons/collections/list.h>
#include <stdio.h>
#include "planificadorCorto.h"
#include "../../mappaLib/include/utils.h"

t_msj_kernel_cpu esperar_cpu();
char** recibir_parametros_de_instruccion();

#endif /* INCLUDE_COMUNICACIONES_KERNEL_H_ */
