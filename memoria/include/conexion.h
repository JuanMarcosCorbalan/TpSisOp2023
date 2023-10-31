#ifndef CONEXION_H_
#define CONEXION_H_

#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include "include/main.h"
#include "../../mappaLib/include/sockets.h"
#include "../../mappaLib/include/protocolo.h"
#include "../../mappaLib/include/utils.h"

#define MAX_LINE_LENGTH 100

typedef struct {
    int fd_cliente;
} t_procesar_cliente_args;


typedef struct {
	int pid;
	t_list* instrucciones;
} t_proceso_instrucciones;

int experar_clientes(int server_socket);
t_list* generar_instrucciones(char* path);
codigo_instruccion instruccion_to_enum(char* instruccion);
void iniciar_proceso_memoria(char* path, int size, int pid);
t_instruccion* recibir_pedido_instruccion(int socket_cpu);
void procesar_pedido_instruccion(int socket_cpu);
t_instruccion* buscar_instruccion(int pid, int program_counter);

#endif
