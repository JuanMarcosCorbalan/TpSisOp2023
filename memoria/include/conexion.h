#ifndef CONEXION_H_
#define CONEXION_H_

#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include "../../mappaLib/include/sockets.h"
#include "../../mappaLib/include/protocolo.h"
#include "../../mappaLib/include/utils.h"

#define MAX_LINE_LENGTH 100


typedef struct {
    int fd_cliente;
    t_log* logger;
} t_procesar_cliente_args;


typedef struct {
	int pid;
	t_list* instrucciones;
} t_proceso_instrucciones;

int experar_clientes(t_log* logger, int server_socket, t_config* config);
t_list* generar_instrucciones(char* path);
codigo_instruccion instruccion_to_enum(char* instruccion);
void iniciar_proceso_memoria(char* path, int size, int pid, int socket_kernel, t_log* logger);
t_instruccion* recibir_pedido_instruccion(int socket_cpu);
void procesar_pedido_instruccion(int socket_cpu, t_list* proceso_instrucciones);
t_instruccion* buscar_instruccion(int pid, int program_counter, t_list* proceso_instrucciones);

char* inicializar_bitmap_marcos(void);
void procesar_solicitud_marco(int fd_cpu);

void cargar_pagina(int pid, int numero_pagina);
void realizar_reemplazo(int pid, int numero_pagina);

uint32_t leer_espacio_usuario(int direccion);
void escribir_espacio_usuario(int direccion, uint32_t valor);
#endif
