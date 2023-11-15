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

t_queue* procesos_en_new;
t_queue* procesos_en_exec;
t_list* procesos_en_ready;
t_list* procesos_en_blocked;
t_list* procesos_en_exit;

pthread_t* hilo_largo_plazo;
pthread_t* hilo_corto_plazo;

pthread_mutex_t mutex_ready_list;
pthread_mutex_t mutex_cola_new;
pthread_mutex_t mutex_cola_exec;
pthread_mutex_t mutex_logger;
pthread_mutex_t mutex_planificacion_activa;

sem_t sem_multiprogramacion;
sem_t sem_procesos_new;
sem_t sem_procesos_ready;
sem_t sem_proceso_exec;
sem_t sem_procesos_exit;

int asignador_pid;
int asignador_iid;

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
void iniciar_consola(t_log* logger, t_config* config, int fd_memoria);
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

#endif
