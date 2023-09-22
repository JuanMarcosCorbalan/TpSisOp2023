#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include "conexion.h"
#include "../include/sockets.h"
#include "../include/protocolo.h"
#include "../include/utils.h"

#define MAX_LINE_LENGTH 100

t_config* iniciar_config(void);
t_list* generar_instrucciones(char* path);
codigo_instruccion instruccion_to_enum(char* instruccion);

#endif
