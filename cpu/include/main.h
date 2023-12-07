#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../../mappaLib/include/sockets.h"
#include "../../mappaLib/include/protocolo.h"
#include "../../mappaLib/include/utils.h"

typedef struct {
	char* ip_memoria;
	char* puerto_memoria;
	char* puerto_escucha_dispatch;
	char* puerto_escucha_interrupt;
} t_config_cpu;

t_config_cpu config_cpu;

sem_t sem_nuevo_proceso;
sem_t sem_ciclo_de_instrucciones;

t_instruccion* recibir_instruccion();
void* ejecutar_ciclo_instruccion(void *arg);
t_config* iniciar_config(void);
void* ejecutar_pcb(void *arg);
void ejecutar_instrucciones(t_pcb* pcb);
void* ejecutar_interrupcion(t_pcb* pcb, void *arg);
void fetch(t_pcb* pcb);
t_instruccion* solicitar_instruccion(int pid, int program_counter);
void decode(t_instruccion* instruccion, t_pcb* pcb);
void leer_config();
void ejecutar_set(t_pcb* pcb, char* param1, char* param2);
void ejecutar_sum(t_pcb* pcb, char* param1, char* param2);
void ejecutar_sub(t_pcb* pcb, char* param1, char* param2);
void ejecutar_wait(t_pcb* pcb, char* param1);
void ejecutar_exit(t_pcb* pcb);
void ejecutar_fopen(t_pcb* pcb, char* param1, char* param2);
void ejecutar_fclose(t_pcb* pcb, char* param1);
void ejecutar_fseek(t_pcb* pcb,char* param1, char* param2);
void ejecutar_fread(t_pcb* pcb, char* param1, char* param2);
void ejecutar_fwrite(t_pcb* pcb, char* param1, char* param2);
void ejecutar_ftruncate(t_pcb* pcb, char* param1, char* param2);

	}
#endif
