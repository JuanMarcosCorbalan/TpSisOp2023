#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<commons/log.h>
#include<commons/config.h>
#include<readline/readline.h>
#include "utils.h"

extern t_log* logger;

typedef enum
{
	MENSAJE,
	PAQUETE,
	PCB,
	INTERRUPCION,
	DATOS_PROCESO_NEW
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


void* serializar_paquete(t_paquete* paquete, int bytes);
t_paquete* crear_paquete(void);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
t_list* recibir_paquete(int socket_cliente);
void paquete(int conexion);
void leer_consola(t_log* logger);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void eliminar_paquete(t_paquete* paquete);

void send_datos_proceso(char* path, int size_proceso, int pid, int socket_cliente);
void send_interrupt(t_interrupt interrupcion, int fd);

t_interrupt* recv_interrupcion(int fd);


void enviar_mensaje(char* mensaje, int socket_cliente);
void recibir_mensaje(int socket_cliente);

int recibir_operacion(int socket_cliente);

void crear_buffer(t_paquete* paquete);
void* recibir_buffer(int* size, int socket_cliente);

void iterator(char* value);

#endif
