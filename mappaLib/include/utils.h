#ifndef MAPPALIB_H_
#define MAPPALIB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/queue.h>
#include <semaphore.h>
#include <inttypes.h>
#include <stdbool.h>
#include <pthread.h>

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
	t_registros_generales_cpu registros_generales_cpu;
	t_archivos_abiertos archivos_abiertos;
	estado estado;

	double tiempo_inicial_ejecucion;
} t_pcb;

typedef enum {
	SET,
	SUM,
	SUB,
	EXIT,
} codigo_instruccion;

typedef struct {
	codigo_instruccion codigo;
	char* param1;
	char* param2;
} t_instruccion;

typedef enum{

	// los que usa solo CPU
	PCB_A_EJECUTAR,

	SET,
	MOV_IN,
	MOV_OUT,

	// las que usa CPU y MEMORIA
	F_OPEN,
	F_CLOSE,
	F_SEEK,
	F_TRUNCATE,
	F_READ,
	F_WRITE,
	WAIT,
	SIGNAL,

	// las que usa solo MEMORIA

} t_msj_kernel_cpu;


void imprimirPrueba();

#endif
