#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<commons/log.h>
#include<commons/config.h>
#include<readline/readline.h>
#include "utils.h"

extern t_list* datos_procesos;
extern t_log* logger;

typedef enum
{
	MENSAJE,
	PAQUETE,
	PCB,
	INTERRUPCION,
	SOLICITAR_INSTRUCCION,
	PROXIMA_INSTRUCCION,
	DATOS_PROCESO_NEW,
	EJECUTAR_PCB,
	PCB_ACTUALIZADO,
	TDP,
	HANDSHAKE_CPU_MEMORIA
}op_code;

typedef enum
{
	END_PROCESO,
	MOTIVO2,
	MOTIVO3,
	MOTIVOX
}interrupt_code;

typedef struct
{
	interrupt_code motivo;
	int interrupt_id;
	int flag;
}t_interrupt;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef struct
{
	int pid;
	int program_counter;
} t_solicitud_instruccion;

t_paquete* crear_paquete(op_code codigo_operacion);
void* serializar_paquete(t_paquete* paquete, int bytes);
//t_paquete* crear_paquete(void);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
t_list* recibir_paquete(int socket_cliente);
//void paquete(int conexion);
void leer_consola(t_log* logger);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void eliminar_paquete(t_paquete* paquete);
void send_datos_proceso(char* path, int size_proceso, int pid, int fd);
t_datos_proceso* recv_datos_proceso(int fd);
void send_interrupcion(t_interrupt* interrupcion, int fd);
t_interrupt* recv_interrupcion(int fd);
void enviar_mensaje(char* mensaje, int socket_cliente);
void recibir_mensaje(int socket_cliente);

int recibir_operacion(int socket_cliente);

void crear_buffer(t_paquete* paquete);
void* recibir_buffer(int* size, int socket_cliente);

void iterator(char* value);

// SOLICITAR_INSTRUCCION
void send_solicitar_instruccion(int fd, int pid, int program_counter);
t_solicitud_instruccion* recv_solicitar_instruccion(int fd);

// PROXIMA_INSTRUCCION
void send_proxima_instruccion(int fd, t_instruccion* instruccion);
t_instruccion* recv_proxima_instruccion(int fd);

// EJECUTAR_PCB
void send_ejecutar_pcb(int fd, t_pcb* pcb);
t_pcb* recv_ejecutar_pcb(int fd);

// PCB_ACTUALIZADO
void send_pcb_actualizado(int fd, t_pcb* pcb);
t_pcb* recv_pcb_actualizado(int fd);

//TDP
void send_tdp(int fd, t_tdp* tdp);
t_tdp* recv_tdp(int fd);

//HANDSHAKE CPU MEMORIA
void send_herramientas_traduccion(int fd, size_t tam_pag, void* espacio_usuario);
t_herramientas_traduccion* recv_herramientas_traduccion(int fd);

#endif
