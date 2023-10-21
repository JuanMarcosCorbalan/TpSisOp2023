#ifndef MAPPALIB_H_
#define MAPPALIB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include <semaphore.h>

typedef enum {
	NEW,
	READY,
	EXEC,
	BLOCKED,
	EXIT_ESTADO
} estado;

typedef struct {
	int fd;
	uint32_t f_pointer;
} t_archivos_abiertos;

typedef struct {
	uint32_t ax;
	uint32_t bx;
	uint32_t cx;
	uint32_t dx;

} t_registros_generales_cpu;

typedef struct {
	int pid;
	int program_counter;
	int prioridad;
	estado estado;
	t_registros_generales_cpu registros_generales_cpu;
//	t_archivos_abiertos archivos_abiertos;

} t_pcb;

typedef enum {
	SET,
	SUM,
	SUB,
	SLEEP,
	WAIT,
	SIGNAL,
	EXIT,
} codigo_instruccion;

typedef struct {
	codigo_instruccion codigo;
	char* param1;
	char* param2;
} t_instruccion;

typedef struct
{
	int pid;
	char* path;
	int size;
}t_datos_proceso;

//t_list* datos_procesos;
void imprimirPrueba();
void* list_pop_con_mutex(t_list* lista, pthread_mutex_t* mutex);
void list_push_con_mutex(t_list* lista, void* elemento, pthread_mutex_t* mutex);
void* queue_pop_con_mutex(t_queue* queue, pthread_mutex_t* mutex);
void queue_push_con_mutex(t_queue* queue, void* elemento, pthread_mutex_t* mutex);

#endif
