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



int experar_clientes(t_log* logger, int server_socket);
void inicializar_variables(t_log* logger, t_config* config);
t_list* generar_instrucciones(char* path);
codigo_instruccion instruccion_to_enum(char* instruccion);
void iniciar_proceso_memoria(char* path, int size, int pid, int socket_kernel);
t_instruccion* recibir_pedido_instruccion(int socket_cpu);
void procesar_pedido_instruccion(int socket_cpu, t_list* proceso_instrucciones);
t_instruccion* buscar_instruccion(int pid, int program_counter, t_list* proceso_instrucciones);
t_config* iniciar_config(void);

char* inicializar_bitmap_marcos(void);
void procesar_solicitud_marco(int fd_cpu);
t_pagina* buscar_pagina(int pid, int numero_pagina);
void cargar_pagina(int pid, int numero_pagina, int desplazamiento);
void realizar_reemplazo(t_pagina* pagina, int direccion, uint32_t valor);
void efectivizar_carga(int marco, t_pagina* pagina, int direccion, uint32_t valor);
void descargar_pagina(t_pagina* pagina, int direccion);

uint32_t leer_espacio_usuario(int direccion, int pid);
void escribir_espacio_usuario(int direccion, uint32_t valor, int pid, int numero_pagina);
uint32_t recibir_valor_bloque();
uint32_t* recibir_bloques_reservados();

void finalizar_proceso(t_pcb* pcb);
#endif
