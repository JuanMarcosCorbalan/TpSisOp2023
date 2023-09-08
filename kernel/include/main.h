#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include "../include/sockets.h"
#include "../include/protocolo.h"

t_log* iniciar_logger(void);
t_config* iniciar_config(void);
bool conectar_modulos(t_log* logger, t_config* config, int* fd_cpu, int* fd_filesystem, int* fd_memoria);
void terminar_programa(int conexion, int conexion2, int conexion3, t_log* logger, t_config* config);

#endif
