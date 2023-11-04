#ifndef SOCKETS_H_
#define SOCKETS_H_

#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>

int iniciar_servidor(char*);
int esperar_cliente(t_log* logger, int socket_servidor);
int crear_conexion(t_log* logger, char* ip, char* puerto);
void liberar_conexion(int socket_cliente);

#endif
