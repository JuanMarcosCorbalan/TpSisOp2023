#ifndef MAPPALIB_H_
#define MAPPALIB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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

void imprimirPrueba();

#endif
