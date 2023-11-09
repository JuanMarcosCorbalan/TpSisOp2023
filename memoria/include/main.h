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
int retardo_respuesta;
int tam_pagina;
int tam_memoria;
int cant_marcos;
char* bitmap_marcos;
char* inicializar_bitmap_marcos(void);

void* espacio_usuario;

#endif
