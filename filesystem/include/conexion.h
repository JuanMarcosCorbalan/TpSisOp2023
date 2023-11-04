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

int experar_clientes(t_log* logger, int server_socket);

#endif
