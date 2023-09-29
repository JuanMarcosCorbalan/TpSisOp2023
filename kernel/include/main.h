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
#include "../include/sockets.h"
#include "../include/protocolo.h"
#include "../include/conexion.h"
#include "../include/utils.h"

t_queue* procesos_en_new;
t_list* procesos_en_ready;
t_list* procesos_en_blocked;
t_list* procesos_en_exit;

pthread_t* hilo_largo_plazo;

int asignador_pid;
int asignador_iid;
sem_t* cont_multiprogramacion;
sem_t* bin_proceso_new;

t_log* iniciar_logger(void);
t_config* iniciar_config(void);
bool conectar_modulos(t_log* logger, t_config* config, int* fd_cpu, int* fd_filesystem, int* fd_memoria);
void terminar_programa(int conexion, int conexion2, int conexion3, t_log* logger, t_config* config);

void iniciar_proceso(t_log* logger, char *args[]);
void finalizar_proceso(t_log* logger, t_pcb* proceso_a_finalizar,char *args[], int fd_memoria, int fd_cpu_interrupt);

t_pcb* crear_pcb(char* path, int prioridad);
t_interrupt* crear_interrupcion(interrupt_code motivo);

void inicializar_variables();
void iniciar_consola(t_log* logger);

#endif
