#ifndef MAINKERNEL_H_
#define MAINKERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include "conexion.h"
#include "../../mappaLib/include/sockets.h"
#include "../../mappaLib/include/protocolo.h"
#include "../../mappaLib/include/utils.h"

typedef struct{
	char* recurso;
	int id;
	int instancias;
	t_list* cola_block_asignada;
	pthread_mutex_t mutex_asignado;
}t_recurso;

typedef struct {
    t_pcb* pcb;
    int retardo_bloqueo;
} t_datos_hilo_sleep;

t_queue* procesos_en_new;
t_queue* procesos_en_exec;
t_list* procesos_en_ready;
t_list* procesos_en_blocked;
t_list* procesos_en_blocked_sleep;
t_list* procesos_en_exit;
t_list* lista_recursos;

pthread_t* hilo_largo_plazo;
pthread_t* hilo_corto_plazo;

pthread_mutex_t mutex_ready_list;
pthread_mutex_t mutex_cola_new;
pthread_mutex_t mutex_cola_exec;
pthread_mutex_t mutex_planificacion_activa;
pthread_mutex_t mutex_lista_blocked;
pthread_mutex_t mutex_lista_blocked_sleep;
pthread_mutex_t mutex_lista_exit;
pthread_mutex_t mutex_asignacion_recursos;
pthread_mutex_t mutex_logger;
pthread_mutex_t mutex_lectura_escritura;

sem_t sem_multiprogramacion;
sem_t sem_procesos_new;
sem_t sem_procesos_ready;
sem_t sem_proceso_exec;
sem_t sem_procesos_exit;
sem_t sem_vuelta_blocked;
sem_t sem_procesos_blocked;
sem_t sem_procesos_blocked_sleep;
sem_t sem_asignacion_recursos;
sem_t sem_vuelta_asignacion_recursos;

int asignador_pid;
int asignador_iid;

t_list cola_peticiones_fopen;
t_list* tabla_global_archivos_abiertos;

t_log* iniciar_logger(void);
t_config* iniciar_config(void);
bool conectar_modulos(t_log* logger, t_config* config, int* fd_cpu_dispatch, int* fd_cpu_interrupt, int* fd_filesystem, int* fd_memoria);
void terminar_programa(int conexion, int conexion2, int conexion3, int conexion4, t_log* logger, t_config* config);
void iniciar_proceso(t_log* logger, char *args[], int fd_memoria);
t_pcb* crear_pcb(int prioridad);
void finalizar_proceso(char *args[]);
//void finalizar_proceso(t_log* logger, t_pcb* proceso_a_finalizar,char *args[], int fd_memoria, int fd_cpu_interrupt);
t_interrupt* crear_interrupcion(interrupt_code motivo);

void inicializar_variables();
void* iniciar_consola();
void iniciar_planificacion();
void detener_planificacion();
void pasar_a_ready(t_pcb* proceso);
void planificador_largo_plazo();
void planificar_procesos_ready();
void planificador_corto_plazo();
void planificar_proceso_exec();
t_pcb* obtenerProximoAEjecutar();
void procesar_respuesta_cpu();
void log_cola_ready();
t_list* obtener_lista_pid(t_list* lista);
bool comparar_por_prioridad(void *pcb1, void *pcb2);
void cambiar_estado(t_pcb* pcb, estado estado);
void pcb_destroy(t_pcb* pcb);
void inicializar_semaforos();
void semaforos_destroy();
t_pcb* buscar_proceso_en_queue(int pid, t_queue* queue);
t_pcb* buscar_proceso(int pid);
t_pcb* buscar_proceso_en_list(int pid, t_list* lista);
bool is_pid_equal(void *element, int target_pid);
void actualizar_multiprogramacion(char *args[]);
void listar_estados_proceso();
t_pcb* pcb_copy_function(t_pcb* original);
t_list* queue_to_list_copy(t_queue* original);

void procesar_cambio_estado(t_pcb* pcb, estado nuevo_estado);
void procesar_exit();
t_list* inicializar_recursos();
int* string_to_int_array(char** array_de_strings);
t_recurso* buscar_recurso(char* recurso);
void atender_wait(t_pcb* pcb, char* recurso);
void procesar_vuelta_blocked();
void atender_sleep(t_pcb* pcb, int retardo_bloqueo);
void atender_signal(t_pcb* pcb, char* recurso);
void procesar_sleep(void* args);
void recurso_destroy(t_recurso* recurso);
void agregar_recurso(char* recurso, t_pcb* pcb);
void quitar_recurso(char* recurso, t_pcb* pcb);
t_list* iniciar_recursos_en_proceso();
void liberar_recursos(t_pcb* proceso);
t_pcb* buscar_proceso_en_list(int pid, t_list* lista);
t_pcb* buscar_proceso_a_finalizar(int target_pid);
bool buscar_por_nombre(void* elemento, void* nombre_buscado);
void inicializar_lock(t_lock* lock);
void bloquear_lectura(t_lock* lock);
void bloquear_escritura(t_lock* lock);
void desbloquear(t_lock* lock);

#endif
