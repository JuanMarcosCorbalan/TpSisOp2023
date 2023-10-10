#ifndef CONEXION_H_
#define CONEXION_H_

#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <commons/config.h>
#include <commons/log.h>
#include "../../mappaLib/include/sockets.h"
#include "../../mappaLib/include/protocolo.h"

int esperar_clientes(int server_socket);

#endif
