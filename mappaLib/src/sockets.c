#include "sockets.h"

int iniciar_servidor(char* puerto)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	if (getaddrinfo(NULL, puerto, &hints, &servinfo) != 0) {
	    perror("getaddrinfo");
	    exit(EXIT_FAILURE);
	}

	// Creamos el socket de escucha del servidor

	for(p = servinfo; p != NULL; p = p->ai_next){
		socket_servidor = socket(p -> ai_family, p -> ai_socktype, p -> ai_protocol);
		if(socket_servidor == -1){
			continue;
		}

		// Asociamos el socket a un puerto
		if(bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1){
			close(socket_servidor);
			continue;
		}
		break;
	}
	// Escuchamos las conexiones entrantes
	if (listen(socket_servidor, SOMAXCONN) == -1) {
	    perror("listen");
	    exit(EXIT_FAILURE);
	}

	freeaddrinfo(servinfo);
	log_trace(logger, "Listo para escuchar a mi cliente");

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;
	socklen_t tam_direccion = sizeof(struct sockaddr_in);

	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor,(struct sockaddr*) &dir_cliente, &tam_direccion);

	if (socket_cliente == -1) {
		perror("accept");
		exit(EXIT_FAILURE);
	}

	log_info(logger, "Se conecto un cliente!");

	return socket_cliente;
}

int crear_conexion(t_log* loggerConexion, char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen)){
		log_error(loggerConexion, "Error al conectar el socket");
		freeaddrinfo(server_info);
		return 0;
	}

	log_info(loggerConexion, "Conectado correctamente.");
	freeaddrinfo(server_info);
	return socket_cliente;
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}
