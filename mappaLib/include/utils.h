#ifndef MAPPALIB_H_
#define MAPPALIB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

typedef struct {
    void *data;
    struct t_node *next;
} t_node;

typedef enum {
	NEW,
	READY,
	EXEC,
	BLOCKED,
	EXIT_ESTADO
} estado;

typedef enum{
	PROCESO_ACTIVO,
	SUCCESS,
	INVALID_RESOURCE,
	INVALID_WRITE,
	EXIT_CONSOLA,
	INTERRUPT
} t_motivo_exit;

//typedef struct {
//	int fd;
//	uint32_t f_pointer;
//} t_archivos_abiertos;

typedef struct {
	char* nombre_archivo;
	uint32_t puntero;
}t_archivo_abierto_proceso;

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
	t_motivo_exit motivo_exit;
	t_list* archivos_abiertos_proceso;
	t_list* recursos_asignados;
} t_pcb;

typedef struct {
	char* nombre_recurso;
	int instancias;
} t_recurso_asignado;

//typedef struct {
//	int pid;
//	int instancias;
//}t_pid_instancias_recurso;

typedef enum {
	SET,
	SUM,
	SUB,
	JNZ,
	SLEEP,
	WAIT,
	SIGNAL,
	EXIT,
	F_OPEN,
	F_CLOSE,
	F_SEEK,
	F_WRITE,
	F_READ,
	F_TRUNCATE
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

typedef struct
{
	char* nombre_archivo;
	int tamanio_archivo; // en bytes
	int bloque_inicial; // tambien en bytes
}t_fcb;

typedef struct
{
	char* nombre_archivo;
	t_list* locks;
}t_archivo_abierto_global;

// Estructura para representar un lock
//typedef struct {
//    pthread_mutex_t mutex;  // Mutex para la exclusión mutua
//    pthread_cond_t cond;    // Condición para sincronizar los threads
//    int participantes;      // Número de participantes en el lock
//    int encolados;          // Número de threads encolados esperando
//} t_lock;


//t_list* datos_procesos;
void imprimirPrueba();
char* list_to_string(t_list *list);
char* motivo_to_string(t_motivo_exit estado_exit);
char* estado_to_string(estado estado);
void* list_pop_con_mutex(t_list* lista, pthread_mutex_t* mutex);
void list_push_con_mutex(t_list* lista, void* elemento, pthread_mutex_t* mutex);
void* queue_pop_con_mutex(t_queue* queue, pthread_mutex_t* mutex);
void queue_push_con_mutex(t_queue* queue, void* elemento, pthread_mutex_t* mutex);
bool queue_filter(t_queue *queue, bool (*condition)(void *, int), int target_pid);
t_pcb *queue_find_and_remove(t_queue *queue, int target_pid);

#endif
