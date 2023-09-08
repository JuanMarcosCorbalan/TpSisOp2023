#ifndef SOCKETS_H_
#define SOCKETS_H_

#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>

extern t_log* logger;

int iniciar_servidor(char*);
int esperar_cliente(int);
int crear_conexion(t_log* logger, char* ip, char* puerto);
void liberar_conexion(int socket_cliente);

#endif
