#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<commons/log.h>
#include<commons/config.h>
#include<readline/readline.h>
#include "utils.h"

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
	ATENDER_WAIT,
	ATENDER_SIGNAL,
	CAMBIAR_ESTADO,
	ATENDER_SLEEP,
	TDP,
	HANDSHAKE_CPU_MEMORIA,
	RECURSO_WAIT,
	LEER_MEMORIA,
	ESCRIBIR_MEMORIA,
	SOLICITUD_MARCO,
	MARCO,
	PCB_PF,
	CARGAR_PAGINA,
	PAGINA_CARGADA,
	VALOR_LEIDO,
	SOLICITUD_BLOQUES_SWAP,
	VALOR_EN_BLOQUE_IDA,
	VALOR_EN_BLOQUE_VUELTA,
	PETICION,
	FOPEN,
	FCLOSE,
	FSEEK,
	FTRUNCATE,
	FREAD,
	FWRITE,
	INICIARPROCESO,
	FINALIZARPROCESO,
	FIN_FOPEN,
	FIN_FWRITE,
	FIN_FREAD,
	FIN_FTRUNCATE,
	HANDSHAKE_FS_MEMORIA,
	TAMANIO_PAGINA
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

typedef struct {
	char* nombre_archivo;
	char* modo_apertura;
	uint32_t posicion;
	int direccion_fisica;
	uint32_t tamanio;
} t_peticion;

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
void send_interrupcion(t_motivo_exit motivo, int fd);
t_motivo_exit recv_interrupcion(int fd);
void enviar_mensaje(char* mensaje, int socket_cliente);
void recibir_mensaje(t_log* logger, int socket_cliente);

void enviar_operacion(op_code codigo_operacion, int socket_cliente);
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

// PCB
void send_pcb(t_pcb* pcb, int socket);
t_pcb* recv_pcb(int socket);

void empaquetar_recursos(t_paquete* paquete, t_list* lista_de_recursos);
t_list* desempaquetar_recursos(t_list* paquete, int comienzo);

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
void send_handshake_cpu_memoria(int fd_memoria);
int recv_handshake_cpu_memoria(int fd_cpu);

void send_tam_pagina(int fd, int tam_pag);
int recv_tam_pagina(int fd);

//SOLICITAR BLOQUES SWAP
void send_solicitud_bloques_swap(int fd_filesystem, int cant_paginas);
int recv_solicitud_bloques_swap(int fd_memoria);

uint32_t* recv_lista_bloques_reservados(int socket);


// RECURSO_WAIT
void send_recurso_wait(char* recurso, int dispatch_cliente_fd);
char* recv_recurso(int dispatch_cliente_fd);

// RECURSO_SIGNAL
void send_recurso_signal(char* recurso, int dispatch_cliente_fd);

// CAMBIAR_ESTADO
void send_cambiar_estado(estado estado, int fd_modulo);
estado recv_cambiar_estado(int fd_modulo);

//SLEEP
void send_sleep(int tiempo_bloqueado, int fd_modulo);
int recv_sleep(int fd_modulo);

//SOLICITUD DE MARCO
void send_solicitud_marco(int dispatch_cliente_fd, int pid, int numero_pagina);// cpu
pid_y_numpag* recv_solicitud_marco(int dispatch_cliente_fd); //memoria

void send_marco (int dispatch_cliente_fd, int marco); //MEMORIA
int recv_marco (int dispatch_cliente_fd); //cpu

//PAGE FAULT CPU A KERNEL
void send_pcb_pf(int numero_pagina, int desplazamiento, int dispatch_cliente_fd);
numpag_despl* recv_pcb_pf(int fd_cpu_dispatch);

//NUMERO DE PAGINA
void send_numero_pagina(int pid, int numero_pagina, int desplazamiento, int fd_memoria);
pid_numpag_despl* recv_numero_pagina(int fd_kernel);

//PAGINA CARGADA
void send_pagina_cargada(int fd_kernel);
int recv_pagina_cargada(int fd_memoria);

//SOLICITUD DE LECTURA
void send_solicitud_lectura_memoria(int direccion_fisica, int fd_memoria);
int recv_solicitud_lectura_memoria(int fd_cpu);

//ENVIAR VALOR LEIDO
void send_valor_leido_memoria(uint32_t valor, int fd_cpu);
uint32_t recv_valor_leido_memoria(int fd_memoria);

//SOLICITUD DE ESCRITURA
void send_solicitud_escritura_memoria(int direccion_fisica, uint32_t valor, pid_y_numpag*, int fd_memoria);
direccion_y_valor* recv_solicitud_escritura_memoria(int fd_cpu);

//SOLICITUD DE VALOR EN BLOQUE
void send_solicitud_valor_en_bloque(int fd_filesystem, int direccion_bloque);
int recv_solicitud_valor_en_bloque(int fd_memoria);

void send_valor_en_bloque(int fd_memoria, uint32_t valor);
uint32_t recv_valor_en_bloque(int fd_filesystem);
void send_tam_pagina(int tam_pagina, int socket);
int recv_tam_pagina(int fd);

void free_recurso_asignado(void* elemento);

// INFORMACION_ARCHIVO_BLOQUES
void send_datos_archivo_bloques(int , char*,  int);

// FUNCIONES PARA FS
void send_peticion(int socket, t_pcb* pcb ,t_peticion* peticion, op_code codigo_operacion);
void send_peticion_f_close(int socket, t_pcb* pcb ,t_peticion* peticion, op_code codigo_operacion);

uint32_t recv_posicion(int socket);
uint32_t recv_tamanio(int socket);
int recv_dir_logica(int socket);
t_peticion* recv_peticion(int socket);

void send_bloques_reservados(int socket, uint32_t* lista_bloques_reservados, int tamanio);
char* recv_parametros_fopen(int socket);
t_list* recv_parametros(int socket);
void send_solicitud_lectura(int direccion_fisica, int fd_memoria);
int recv_solicitud_lectura(int fd_cpu);
uint32_t* recv_valor_leido(int fd_memoria);

void send_finalizo_fopen(int socket, int numero);
int recv_finalizo_fopen(int socket);
void send_finalizo_ftruncate(int socket);
void recv_finalizo_ftruncate(int socket);
void send_finalizo_fread(int socket);
void recv_finalizo_fread(int socket);
void send_finalizo_fwrite(int socket);
void recv_finalizo_fwrite(int socket);

void send_handshake_fs_memoria(int fd_memoria);
int recv_handshake_fs_memoria(int fd_filesystem);

#endif
