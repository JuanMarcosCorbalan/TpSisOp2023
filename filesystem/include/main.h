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

t_bloque* inicializar_fat(int);
void manejar_peticiones();
t_config* crear_archivo_fcb(char*);
void crear_fcb(char* ,int ,int);
void crear_archivo_en_fat(char*);
void inicializar_fcb(t_fcb*);
void crear_archivo_bloques(char*);
int abrir_archivo(char*);
void leer_archivo(char* ,int, int);
void truncar_archivo(t_config* ,int ,t_bloque*);
void agrandar_archivo(t_config* ,int ,t_bloque*);
void reducir_archivo(t_config* ,int ,t_bloque*);
int ultimo_bloque_archivo_fat( t_config*, t_bloque*);
bool archivo_sin_bloques(t_config*);
void desasignar_bloques_archivo(t_config*, int, t_bloque*, int);
int ultimo_bloque_archivo_fat( t_config* , t_bloque* );
void asignar_bloques_archivo( t_config* , int , t_bloque* , int );
int buscar_bloque_libre_fat(t_bloque* );
int* obtener_lista_bloques_archivo(t_config* , t_bloque* , int ,int );
uint32_t obtener_bloque_siguiente(int , t_bloque* );
char* leer_datos_bloque_archivo(t_config* , int );
void escribir_archivo(char* , int , int , char* , int , t_bloque* );

#endif

