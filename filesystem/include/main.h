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

// cosas para conexion
int fd_memoria = 0;
int fd_cliente = 0;
int socket_cliente;
char* path_fat;

typedef enum {
	ABRIR_ARCHIVO_FS,
	TRUNCAR_ARCHIVO_FS,
	LEER_ARCHIVO_FS,
	ESCRIBIR_ARCHIVO_FS,
	INICIAR_PROCESO_FS,
	FINALIZAR_PROCESO_FS,
}codigo_operacion_fs;

typedef struct {
	uint32_t* array_bloques;
	size_t cantidad_bloques_a_liberar;
}t_bloques_swap;

typedef struct{
	codigo_operacion_fs cod_op;
	char* nombre;
	char* buffer_escritura;
	int tamanio;
	int dir_fisica;
	int puntero;
	int cantidad_bloques_swap;
	t_bloques_swap bloques_ocupados_swap;
}t_operacion;


t_config* iniciar_config(void);

void manejar_peticiones();
t_config* crear_archivo_fcb(char*);
void crear_archivo_fat(const char* , int );
void crear_fcb(char* ,int ,int);
void crear_archivo_en_fat(char*);
void inicializar_fcb(t_fcb*);
void crear_archivo_bloques(char*);
int abrir_archivo(char*);
void leer_archivo(char* , int , int , char*);
void truncar_archivo(char* , int ,const char* );
void agrandar_archivo(t_config* , int ,const char* );
void reducir_archivo(t_config* , int ,const char* );
bool archivo_sin_bloques(t_config*);
void desasignar_bloques_a_archivo(const char* , t_config* , int , int);
void asignar_bloques_a_archivo(const char* , t_config* ,  int , int);
uint32_t* obtener_bloques_asignados(const char* , t_config* );
char* leer_datos_bloque_archivo(t_config* , int );
void escribir_archivo(char* , int , int , char* , int ,const char* );
uint32_t buscar_primer_bloque_libre_fat(FILE*);
uint32_t  buscar_bloque_en_fat (t_config* , int ,const char* );
void procesar_conexion();
uint32_t* reservar_bloques_swap(int );
void liberar_bloques_swap(t_bloques_swap );
uint32_t buscar_primer_bloque_libre_swap(FILE* );


#endif

