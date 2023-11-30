#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/log.h>
#include "conexion.h"
#include "../../mappaLib/include/sockets.h"
#include "../../mappaLib/include/protocolo.h"
#include "../../mappaLib/include/utils.h"

t_config* iniciar_config(void);

uint32_t* inicializar_fat(int);
t_config* crear_archivo_fcb(char*);
void crear_fcb(char* , int , int);
void crear_archivo_en_fat(char*);
void inicializar_fcb(t_fcb*);
void crear_archivo_bloques(int , char*){

#endif

