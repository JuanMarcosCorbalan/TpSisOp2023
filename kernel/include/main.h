#ifndef MAINKERNEL_H_
#define MAINKERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include "../../mappaLib/include/sockets.h"
#include "../../mappaLib/include/protocolo.h"
#include "../../mappaLib/include/conexion.h"
#include "../../mappaLib/include/utils.h"

t_queue* procesos_en_new;
t_list* procesos_en_ready;
t_list* procesos_en_blocked;
t_list* procesos_en_exit;

pthread_t* hilo_largo_plazo;

int asignador_pid;

sem_t* cont_multiprogramacion;
sem_t* bin_proceso_new;

t_log* iniciar_logger(void);
t_config* iniciar_config(void);
bool conectar_modulos(t_log* logger, t_config* config, int* fd_cpu_dispatch, int* fd_cpu_interrupt, int* fd_filesystem, int* fd_memoria);
void terminar_programa(int conexion, int conexion2, int conexion3, int conexion4, t_log* logger, t_config* config);

void iniciar_proceso(t_log* logger, char *args[], int fd_memoria);
t_pcb* crear_pcb(int prioridad);

void inicializar_variables();
void iniciar_consola(t_log* logger, t_config* config, int fd_memoria);
void pasar_a_ready(t_pcb* proceso);
void planificador_largo_plazo();

#endif
