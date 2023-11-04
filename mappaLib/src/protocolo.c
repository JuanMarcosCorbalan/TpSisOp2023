#include "../include/protocolo.h"

t_log* logger;

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

//t_paquete* crear_paquete(void)
//{
//	t_paquete* paquete = malloc(sizeof(t_paquete));
//	paquete->codigo_operacion = PAQUETE;
//	crear_buffer(paquete);
//	return paquete;
//}

t_paquete* crear_paquete(op_code codigo_operacion)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo_operacion;
	crear_buffer(paquete);
	return paquete;
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

//void paquete(int conexion)
//{
//	char* leido;
//	t_paquete* paquete = crear_paquete();
//
//	leido = readline("> ");
//	while(strcmp(leido, "")){
//		agregar_a_paquete(paquete, leido, strlen(leido) +1);
//		leido = readline("> ");
//	}
//
//	enviar_paquete(paquete, conexion);
//
//	free(leido);
//	eliminar_paquete(paquete);
//}

void leer_consola(t_log* logger)
{
	char* leido;

	leido = readline("> ");
	while(strcmp(leido, "")){
		log_info(logger, "Se leyo la linea de consola: %s", leido);
		leido = readline("> ");
	}

	free(leido);
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

int recibir_operacion(int socket_cliente)
{
	op_code opc;
	if(recv(socket_cliente, &opc, sizeof(opc), 0) > 0){
		return opc;
	}
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void iterator(char* value) {
	log_info(logger,"%s", value);
}

// SOLICITAR_INSTRUCCION

void send_solicitar_instruccion(int fd, int pid, int program_counter){
	t_paquete* paquete = crear_paquete(SOLICITAR_INSTRUCCION);
	agregar_a_paquete(paquete, &pid, sizeof(int));
	agregar_a_paquete(paquete, &program_counter, sizeof(int));
	enviar_paquete(paquete, fd);
	// TODO Faltan frees??
	eliminar_paquete(paquete);
}

t_solicitud_instruccion* recv_solicitar_instruccion(int fd){
	t_list* paquete = recibir_paquete(fd);
	int* pid;
	int* program_counter;
	t_solicitud_instruccion* solicitud_instruccion_recibida = NULL;

	pid = list_get(paquete, 0);
	program_counter = list_get(paquete, 1);

	solicitud_instruccion_recibida->pid = *pid;
	solicitud_instruccion_recibida->program_counter = *program_counter;

	list_destroy(paquete);

	return solicitud_instruccion_recibida;
}

// PROXIMA_INSTRUCCION

void send_proxima_instruccion(int fd, t_instruccion* instruccion){
	t_paquete* paquete = crear_paquete(PROXIMA_INSTRUCCION);
	agregar_a_paquete(paquete, &instruccion, sizeof(t_instruccion));
	enviar_paquete(paquete, fd);
	// TODO Faltan frees??
	eliminar_paquete(paquete);
}

t_instruccion* recv_proxima_instruccion(int fd){
	t_list* paquete = recibir_paquete(fd);
	t_instruccion* instruccion_recibida = list_get(paquete, 0);

	list_destroy(paquete);

	return instruccion_recibida;
}

// DATOS_PROCESO_NEW

void send_interrupcion(t_interrupt* interrupcion, int fd){
	t_paquete* datos_interrupcion = crear_paquete(INTERRUPCION);
	t_interrupt* interrupcion_a_enviar;

	interrupcion_a_enviar->motivo = interrupcion->motivo;
	interrupcion_a_enviar->interrupt_id = interrupcion->interrupt_id;
	interrupcion_a_enviar->flag = interrupcion->flag;

	agregar_a_paquete(datos_interrupcion, interrupcion_a_enviar, sizeof(interrupcion_a_enviar));


	enviar_paquete(datos_interrupcion, fd);
	eliminar_paquete(datos_interrupcion);
}
t_interrupt* recv_interrupcion(int fd){
	t_list* paquete = recibir_paquete(fd);
	t_interrupt* interrupcion;
//	interrupcion->motivo = list_get(paquete, 0);
//	interrupcion->interrupt_id = list_get(paquete, 1);
//	interrupcion->flag = list_get(paquete, 2);
	list_destroy(paquete);
	return interrupcion;
}

void send_datos_proceso(char* path, int size_proceso, int pid, int fd){
	t_paquete* datos_proceso = crear_paquete(DATOS_PROCESO_NEW);

	t_datos_proceso* proceso = malloc(sizeof(t_datos_proceso));
	proceso->path = path;
	proceso->pid = pid;
	proceso->size = size_proceso;

	agregar_a_paquete(datos_proceso, proceso, sizeof(proceso));

	enviar_paquete(datos_proceso, fd);
	free(proceso);
	eliminar_paquete(datos_proceso);
}

t_datos_proceso* recv_datos_proceso(int fd){
	t_list* paquete = recibir_paquete(fd);
	t_datos_proceso* datos = list_get(paquete, 0);
	list_destroy(paquete);
	return datos;
}

// EJECUTAR_PCB
void send_ejecutar_pcb(int fd, t_pcb* pcb){
	t_paquete* paquete = crear_paquete(EJECUTAR_PCB);
	agregar_a_paquete(paquete, &pcb, sizeof(t_pcb));
	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
}

t_pcb* recv_ejecutar_pcb(int fd){
	t_list* paquete = recibir_paquete(fd);
	t_pcb* pcb = list_get(paquete, 0);

	list_destroy(paquete);

	return pcb;
}

// PCB_ACTUALIZADO

void send_pcb_actualizado(int fd, t_pcb* pcb){
	t_paquete* paquete = crear_paquete(PCB_ACTUALIZADO);
	agregar_a_paquete(paquete, &pcb, sizeof(t_pcb));
	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
}

t_pcb* recv_pcb_actualizado(int fd){
	t_list* paquete = recibir_paquete(fd);
	t_pcb* pcb = list_get(paquete, 0);

	list_destroy(paquete);

	return pcb;
}
// TDP
void send_tdp(int fd, t_tdp* tdp){
	t_paquete* paquete = crear_paquete(TDP);
	agregar_a_paquete(paquete, &tdp, sizeof(t_tdp));
	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
}

t_tdp* recv_tdp(int fd){
	t_list* paquete = recibir_paquete(fd);
	t_tdp* tdp = list_get(paquete, 0);

	list_destroy(paquete);

	return tdp;
}
