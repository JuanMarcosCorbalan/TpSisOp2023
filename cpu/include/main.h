#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../include/sockets.h"
#include "../include/protocolo.h"
#include "../include/utils.h"

typedef struct {
	char* ip_memoria;
	char* puerto_memoria;
	char* puerto_escucha_dispatch;
	char* puerto_escucha_interrupt;
} t_config_cpu;

t_config_cpu config_cpu;

t_config* iniciar_config(void);
void* ejecutar_pcb(void *arg);
void* ejecutar_interrupcion(void *arg);
void leer_config();
void ejecutar_set(t_pcb* pcb, char* param1, char* param2);
void ejecutar_sum(t_pcb* pcb, char* param1, char* param2);
void ejecutar_sub(t_pcb* pcb, char* param1, char* param2);
void ejecutar_exit(t_pcb* pcb);

#endif
