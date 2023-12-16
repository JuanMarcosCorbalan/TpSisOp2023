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
char* ip_memoria;
char* puerto_escucha;
char* puerto_memoria;

int fd_memoria = 0;
int fd_cliente = 0;
int socket_servidor;
int socket_cliente;
int server_fd;

char* path_fat;
char* path_fcb;
char* path_bloques;
char* ip_memoria;

sem_t cantidad_operaciones;
sem_t peticion_completada;
t_list* operaciones_pendientes;
int RETARDO_ACCESO_BLOQUE;
int RETARDO_ACCESO_FAT;
t_config* config;

int cant_bloques_total;
int cant_bloques_swap;
int cant_bloques_fat;
int tam_bloque;
int tamanio_fat;
int tamanio_swap;
int tamanio_archivo_bloques;


pthread_mutex_t mutex_operaciones_pendientes;

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
	uint32_t buffer_escritura;
	int tamanio;
	int dir_fisica;
	int puntero;
	int cantidad_bloques_swap;
	t_list* bloques_ocupados_swap;
}t_operacion;

typedef struct {
	uint32_t bloque_actual;
}t_basta;

t_config* iniciar_config(void);

void manejar_peticiones();
t_config* crear_archivo_fcb(char*);
//void crear_archivo_fat(char* , int );
void inicializar_archivo_fat(char* path_fat, int cantidad_bloques_fat);
void inicializar_archivo_bloques(char* path_bloques);

void crear_fcb(char* ,int ,int);
void crear_archivo_en_fat(char*);
void inicializar_fcb(t_fcb*);
//void crear_archivo_bloques(char*);
int abrir_archivo(char*);
char* leer_archivo(char* , int , int);
void truncar_archivo(char* , int);
void agrandar_archivo(t_config* , int);
void reducir_archivo(t_config* , int);
bool archivo_sin_bloques(t_config*);
void desasignar_bloques_a_archivo( t_config* , int );
void asignar_bloques_a_archivo(t_config* ,  int );
t_list* obtener_bloques_asignados(t_config* , FILE*);
char* leer_datos_bloque_archivo(t_config* , int );
void escribir_archivo(char* , int , int ,uint32_t, int);
void escribir_datos_bloque_archivo(t_config* archivo_fcb, uint32_t bloque_a_escribir, uint32_t buffer_escritura);
uint32_t buscar_primer_bloque_libre_fat(FILE*);
uint32_t  buscar_bloque_en_fat (t_config* , int);
void procesar_conexion();
t_list* reservar_bloques_swap(int );
void liberar_bloques_swap(t_list* );
uint32_t buscar_primer_bloque_libre_swap(FILE* );
t_operacion* crear_operacion(codigo_operacion_fs cod_op, char* nombre_archivo, uint32_t buffer_escritura, int tamanio, int dir_fisica, int puntero, int cantidad_bloques_solicitados_swap, t_list* bloques_ocupados_swap);
void atender_operaciones();
void iniciar_atencion_operaciones();
void realizar_operacion(t_operacion* operacion);
int server_escuchar();
uint32_t obtener_bloque_puntero_en_fat(t_config* archivo_fcb,FILE* archivo_fat, int bloque_puntero_archivo);

#endif

