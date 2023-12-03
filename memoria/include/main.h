#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <unistd.h>
#include "conexion.h"
#include "../../mappaLib/include/sockets.h"
#include "../../mappaLib/include/protocolo.h"
#include "../../mappaLib/include/utils.h"

t_config* iniciar_config(void);

t_config* config;

/*typedef struct {
	int tam_pagina;
	int tam_memoria;
	int retardo_respuesta;
	char* algoritmo_reemplazo;
} t_config_memoria;*/

//t_config_memoria config_memoria;
#endif
