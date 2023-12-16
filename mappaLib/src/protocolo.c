#include "../include/protocolo.h"

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

void recibir_mensaje(t_log* logger, int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

//int recibir_operacion(int socket_cliente)
//{
//	op_code opc;
//	if(recv(socket_cliente, &opc, sizeof(opc), 0) > 0){
//		return opc;
//	}
//	else
//	{
//		close(socket_cliente);
//		return -1;
//	}
//}

//DEL TP0
int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
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

//void iterator(char* value) {
//	log_info(logger,"%s", value);
//}

// SOLICITAR_INSTRUCCION

void send_solicitar_instruccion(int fd, int pid, int program_counter){
	t_paquete* paquete = crear_paquete(SOLICITAR_INSTRUCCION);

	agregar_a_paquete(paquete, &pid, sizeof(int));
	agregar_a_paquete(paquete, &program_counter, sizeof(int));
	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
}

t_solicitud_instruccion* recv_solicitar_instruccion(int fd){
	t_list* paquete = recibir_paquete(fd);
	t_solicitud_instruccion* solicitud_instruccion_recibida = malloc(sizeof(t_solicitud_instruccion));

	int* pid = list_get(paquete, 0);
	solicitud_instruccion_recibida->pid = *pid;
	free(pid);

	int* program_counter = list_get(paquete, 1);
	solicitud_instruccion_recibida->program_counter = *program_counter;
	free(program_counter);

	list_destroy(paquete);
	return solicitud_instruccion_recibida;
}

// PROXIMA_INSTRUCCION

void send_proxima_instruccion(int fd, t_instruccion* instruccion){
	t_paquete* paquete = crear_paquete(PROXIMA_INSTRUCCION);

	agregar_a_paquete(paquete, &(instruccion->codigo), sizeof(codigo_instruccion));
	agregar_a_paquete(paquete, instruccion->param1, strlen(instruccion->param1) + 1);
	agregar_a_paquete(paquete, instruccion->param2, strlen(instruccion->param2) + 1);

	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
}

t_instruccion* recv_proxima_instruccion(int fd){
	t_list* paquete = recibir_paquete(fd);
	t_instruccion* instruccion_recibida = malloc(sizeof(t_instruccion));

	codigo_instruccion* codigo_instruccion = list_get(paquete, 0);
	instruccion_recibida->codigo = *codigo_instruccion;
	free(codigo_instruccion);

	char* parametro1 = list_get(paquete, 1);
	instruccion_recibida->param1 = malloc(strlen(parametro1));
	strcpy(instruccion_recibida->param1, parametro1);
	free(parametro1);

	char* parametro2 = list_get(paquete, 2);
	instruccion_recibida->param2 = malloc(strlen(parametro2));
	strcpy(instruccion_recibida->param2, parametro2);
	free(parametro2);

	list_destroy(paquete);
	return instruccion_recibida;
}

// INTERRUPCION
void send_interrupcion(t_motivo_exit motivo, int fd){
	t_paquete* paquete = crear_paquete(INTERRUPCION);
	agregar_a_paquete(paquete, &motivo, sizeof(motivo));
	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
}

t_motivo_exit recv_interrupcion(int fd){
	t_list* paquete = recibir_paquete(fd);
	t_motivo_exit* motivo = list_get(paquete, 0);
	t_motivo_exit ret = *motivo;
	free(motivo);
	list_destroy(paquete);
	return ret;
}

// DATOS_PROCESO_NEW
void send_datos_proceso(char* path, int size_proceso, int pid, int fd){
	t_paquete* datos_proceso = crear_paquete(DATOS_PROCESO_NEW);

	agregar_a_paquete(datos_proceso, path, strlen(path) + 1);
	agregar_a_paquete(datos_proceso, &size_proceso, sizeof(int));
	agregar_a_paquete(datos_proceso, &pid, sizeof(int));

	enviar_paquete(datos_proceso, fd);
	eliminar_paquete(datos_proceso);
}

t_datos_proceso* recv_datos_proceso(int fd){
	t_list* paquete = recibir_paquete(fd);
	t_datos_proceso* datos = malloc(sizeof(t_datos_proceso));
	char* path = list_get(paquete, 0);
	datos->path = malloc(strlen(path));
	strcpy(datos->path, path);
	free(path);

	int* size = list_get(paquete, 1);
	datos->size = *size;
	free(size);

	int* pid = list_get(paquete, 2);
	datos->pid = *pid;
	free(pid);
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

// PCB
void send_pcb(t_pcb* pcb, int socket){
	t_paquete* paquete = crear_paquete(PCB);

	agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
	agregar_a_paquete(paquete, &(pcb->program_counter), sizeof(int));
	agregar_a_paquete(paquete, &(pcb->prioridad), sizeof(int));
	agregar_a_paquete(paquete, &(pcb->estado), sizeof(estado));

	agregar_a_paquete(paquete, &(pcb->registros_generales_cpu.ax), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(pcb->registros_generales_cpu.bx), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(pcb->registros_generales_cpu.cx), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(pcb->registros_generales_cpu.dx), sizeof(uint32_t));

	agregar_a_paquete(paquete, &(pcb->motivo_exit), sizeof(t_motivo_exit));

	empaquetar_recursos(paquete, pcb->recursos_asignados);

//	empaquetar_archivos_abiertos_proceso(paquete, pcb->archivos_abiertos_proceso);

	enviar_paquete(paquete, socket);
	eliminar_paquete(paquete);
}

void empaquetar_recursos(t_paquete* paquete, t_list* lista_de_recursos){
	int cantidad_recursos = list_size(lista_de_recursos);
	agregar_a_paquete(paquete, &(cantidad_recursos), sizeof(int));
	for(int i = 0; i<cantidad_recursos; i++){
		t_recurso_asignado* recurso_asignado = list_get(lista_de_recursos, i);
		agregar_a_paquete(paquete, recurso_asignado->nombre_recurso, strlen(recurso_asignado->nombre_recurso) + 1);
		agregar_a_paquete(paquete, &(recurso_asignado->instancias), sizeof(int));
	}
}

void empaquetar_archivos_abiertos_proceso(t_paquete* paquete, t_list* lista_archivos_abiertos_proceso){
	int cantidad_archivos_abiertos = list_size(lista_archivos_abiertos_proceso);
	agregar_a_paquete(paquete, &(cantidad_archivos_abiertos), sizeof(int));
	for(int i = 0; i<cantidad_archivos_abiertos; i++){
		t_archivo_abierto_proceso* archivo_abierto_proceso = list_get(lista_archivos_abiertos_proceso, i);
		agregar_a_paquete(paquete, archivo_abierto_proceso->nombre_archivo, strlen(archivo_abierto_proceso->nombre_archivo) + 1);
		agregar_a_paquete(paquete, &(archivo_abierto_proceso->puntero), sizeof(uint32_t));
	}
}
t_list* desempaquetar_recursos(t_list* paquete, int comienzo){
	t_list* recursos = list_create();
	int* cantidad_recursos = list_get(paquete, comienzo);
	int i = comienzo + 1;

	while(i - comienzo - 1 < (*cantidad_recursos* 2)){
		t_recurso_asignado* recurso_asignado = malloc(sizeof(t_recurso_asignado));
		char* nombre = list_get(paquete, i);
		recurso_asignado->nombre_recurso = malloc(strlen(nombre) + 1);
		strcpy(recurso_asignado->nombre_recurso, nombre);
		i++;

		int* instancia = list_get(paquete, i);
		recurso_asignado->instancias = *instancia;
		free(instancia);
		i++;

		list_add(recursos, recurso_asignado);
//		free(recurso_asignado->nombre_recurso);
	}

	free(cantidad_recursos);
	return recursos;
}

t_list* desempaquetar_archivos_abiertos_proceso(t_list* paquete, int comienzo){
	t_list* lista_archivos_abiertos_proceso = list_create();
	int* cantidad_archivos_abiertos = list_get(paquete, comienzo);
	int i = comienzo + 1;

	while(i - comienzo - 1 < (*cantidad_archivos_abiertos* 2)){
		t_archivo_abierto_proceso* archivo_abierto_proceso = malloc(sizeof(t_archivo_abierto_proceso));
		char* nombre_archivo = list_get(paquete, i);
		archivo_abierto_proceso->nombre_archivo = malloc(strlen(nombre_archivo) + 1);
		strcpy(archivo_abierto_proceso->nombre_archivo, nombre_archivo);
		i++;

		uint32_t* puntero = list_get(paquete, i);
		archivo_abierto_proceso->puntero = *puntero;
		free(puntero);
		i++;

		list_add(lista_archivos_abiertos_proceso, archivo_abierto_proceso);
//		free(recurso_asignado->nombre_recurso);
	}

	free(cantidad_archivos_abiertos);
	return lista_archivos_abiertos_proceso;
}

t_pcb* recv_pcb(int socket){
	t_list* paquete = recibir_paquete(socket);
	t_pcb* pcb = malloc(sizeof(t_pcb));

	int* pid = list_get(paquete, 0);
	pcb->pid = *pid;
	free(pid);

	int* program_counter = list_get(paquete, 1);
	pcb->program_counter = *program_counter;
	free(program_counter);

	int* prioridad = list_get(paquete, 2);
	pcb->prioridad = *prioridad;
	free(prioridad);

	estado* estado = list_get(paquete, 3);
	pcb->estado = *estado;
	free(estado);

	uint32_t* ax = list_get(paquete, 4);
	pcb->registros_generales_cpu.ax = *ax;
	free(ax);

	uint32_t* bx = list_get(paquete, 5);
	pcb->registros_generales_cpu.bx = *bx;
	free(bx);

	uint32_t* cx = list_get(paquete, 6);
	pcb->registros_generales_cpu.cx = *cx;
	free(cx);

	uint32_t* dx = list_get(paquete, 7);
	pcb->registros_generales_cpu.dx = *dx;
	free(dx);

	t_motivo_exit* motivo_exit = list_get(paquete, 8);
	pcb->motivo_exit = *motivo_exit;
	free(motivo_exit);

	t_list* recursos = desempaquetar_recursos(paquete, 9);
	pcb->recursos_asignados = recursos;

//	t_list* archivos_abiertos = desempaquetar_archivos_abiertos_proceso(paquete, 13);
//	pcb->archivos_abiertos_proceso = archivos_abiertos;

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

//RECURSO_WAIT

void send_recurso_wait(char* recurso, int dispatch_cliente_fd){
	t_paquete* paquete = crear_paquete(ATENDER_WAIT);
	agregar_a_paquete(paquete, recurso, strlen(recurso) + 1);
	enviar_paquete(paquete, dispatch_cliente_fd);
	eliminar_paquete(paquete);
}

void send_recurso_signal(char* recurso, int dispatch_cliente_fd){
	t_paquete* paquete = crear_paquete(ATENDER_SIGNAL);
	agregar_a_paquete(paquete, recurso, strlen(recurso) + 1);
	enviar_paquete(paquete, dispatch_cliente_fd);
	eliminar_paquete(paquete);
}

char* recv_recurso(int dispatch_cliente_fd){
	t_list* paquete = recibir_paquete(dispatch_cliente_fd);
	char* recurso = list_get(paquete, 0);

	list_destroy(paquete);

	return recurso;
}

//CAMBIAR_ESTADO
void send_cambiar_estado(estado estado, int fd_modulo){
	t_paquete* paquete = crear_paquete(CAMBIAR_ESTADO);
	agregar_a_paquete(paquete, &estado, sizeof(estado));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

estado recv_cambiar_estado(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	estado* estado_proceso = list_get(paquete, 0);
	estado ret = *estado_proceso;
	free(estado_proceso);
	list_destroy(paquete);
	return ret;
}

//SLEEP
void send_sleep(int tiempo_bloqueado, int fd_modulo){
	t_paquete* paquete = crear_paquete(ATENDER_SLEEP);
	agregar_a_paquete(paquete, &tiempo_bloqueado, sizeof(tiempo_bloqueado));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

int recv_sleep(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	int* tiempo_bloqueado_ptr = list_get(paquete, 0);
	int tiempo_bloqueado = *tiempo_bloqueado_ptr;
	free(tiempo_bloqueado_ptr);
	list_destroy(paquete);
	return tiempo_bloqueado;
}

void send_peticion(int socket, t_pcb* pcb ,t_peticion* peticion, op_code codigo_operacion){
	t_paquete* paquete = crear_paquete(FOPEN);
	agregar_a_paquete(paquete, &pcb, sizeof(t_pcb));
	agregar_a_paquete(paquete, peticion->nombre_archivo, strlen(peticion->nombre_archivo) + 1);
	agregar_a_paquete(paquete, peticion->modo_apertura, strlen(peticion->modo_apertura) + 1);
	agregar_a_paquete(paquete, &(peticion->direccion_fisica), sizeof(char));
	agregar_a_paquete(paquete, &(peticion->tamanio), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(peticion->posicion), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(codigo_operacion), sizeof(op_code));

	enviar_paquete(paquete, socket);

	eliminar_paquete(paquete);
}

void send_peticion_f_close(int socket, t_pcb* pcb ,t_peticion* peticion, op_code codigo_operacion){
	t_paquete* paquete = crear_paquete(FCLOSE);
	agregar_a_paquete(paquete, &pcb, sizeof(t_pcb));
	agregar_a_paquete(paquete, peticion->nombre_archivo, strlen(peticion->nombre_archivo) + 1);
	agregar_a_paquete(paquete, peticion->modo_apertura, strlen(peticion->modo_apertura) + 1);
	agregar_a_paquete(paquete, &(peticion->direccion_fisica), sizeof(char));
	agregar_a_paquete(paquete, &(peticion->tamanio), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(peticion->posicion), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(codigo_operacion), sizeof(op_code));

	enviar_paquete(paquete, socket);

	eliminar_paquete(paquete);
}

void send_peticion_f_truncate(int socket,t_peticion_ftruncate* peticion){
	t_paquete* paquete = crear_paquete(FTRUNCATE);
	agregar_a_paquete(paquete, peticion->nombre_archivo, strlen(peticion->nombre_archivo) + 1);
	agregar_a_paquete(paquete, &(peticion->tamanio), sizeof(uint32_t));

	enviar_paquete(paquete, socket);

	eliminar_paquete(paquete);
}

void send_peticion_f_seek(int socket,t_peticion_fseek* peticion){
	t_paquete* paquete = crear_paquete(FSEEK);
	agregar_a_paquete(paquete, peticion->nombre_archivo, strlen(peticion->nombre_archivo) + 1);
	agregar_a_paquete(paquete, &(peticion->posicion), sizeof(uint32_t));

	enviar_paquete(paquete, socket);

	eliminar_paquete(paquete);
}
// CPU Y KERNEL
void send_peticion_f_write(int socket,t_peticion_fwrite* peticion) {
	t_paquete* paquete = crear_paquete(FWRITE);
	agregar_a_paquete(paquete, peticion->nombre_archivo, strlen(peticion->nombre_archivo) + 1);
	agregar_a_paquete(paquete, &(peticion->direccion_fisica), sizeof(int));

	enviar_paquete(paquete, socket);

	eliminar_paquete(paquete);
}
// KERNEL Y FS
void send_peticion_f_write_fs(int socket,t_peticion_fwrite* peticion, uint32_t puntero) {
	t_paquete* paquete = crear_paquete(FWRITE);
	agregar_a_paquete(paquete, peticion->nombre_archivo, strlen(peticion->nombre_archivo) + 1);
	agregar_a_paquete(paquete, &(peticion->direccion_fisica), sizeof(int));
	agregar_a_paquete(paquete, &(puntero), sizeof(uint32_t));

	enviar_paquete(paquete, socket);

	eliminar_paquete(paquete);

}


void send_peticion_f_read(int socket,t_peticion_fread* peticion) {
	t_paquete* paquete = crear_paquete(FREAD);
	agregar_a_paquete(paquete, peticion->nombre_archivo, strlen(peticion->nombre_archivo) + 1);
	agregar_a_paquete(paquete, &(peticion->direccion_fisica), sizeof(int));

	enviar_paquete(paquete, socket);

	eliminar_paquete(paquete);
}

void send_peticion_f_read_fs(int socket,t_peticion_fread* peticion, uint32_t puntero) {
	t_paquete* paquete = crear_paquete(FREAD);
	agregar_a_paquete(paquete, peticion->nombre_archivo, strlen(peticion->nombre_archivo) + 1);
	agregar_a_paquete(paquete, &(peticion->direccion_fisica), sizeof(int));
	agregar_a_paquete(paquete, &(puntero), sizeof(uint32_t));

	enviar_paquete(paquete, socket);

	eliminar_paquete(paquete);

}

t_peticion_ftruncate* recv_peticion_f_truncate(int socket){
	t_list* paquete = recibir_paquete(socket);
	t_peticion_ftruncate* peticion = malloc(sizeof(t_peticion_ftruncate));

	//char* nombre_archivo = list_get(paquete, 0);
	peticion->nombre_archivo = list_get(paquete, 0);


	uint32_t* tamanio_archivo = list_get(paquete, 1);
	peticion->tamanio = *tamanio_archivo;

	free(tamanio_archivo);
	return peticion;

}
t_peticion_fseek* recv_peticion_f_seek(int socket){
	t_list* paquete = recibir_paquete(socket);
	t_peticion_fseek* peticion = malloc(sizeof(t_peticion_fseek));

	peticion->nombre_archivo = list_get(paquete,0);

	uint32_t* posicion = list_get(paquete, 1);
	peticion->posicion = *posicion;


	free(posicion);
	return peticion;
}
// CPU Y KERNEL
t_peticion_fwrite* recv_peticion_f_write(int socket){
	t_list* paquete = recibir_paquete(socket);
	t_peticion_fwrite* peticion = malloc(sizeof(t_peticion_fwrite));

	peticion->nombre_archivo = list_get(paquete,0);

	int* direccion_fisica = list_get(paquete, 1);
	peticion->direccion_fisica = *direccion_fisica;


	free(direccion_fisica);
	return peticion;
}

// KERNEL Y FS
t_peticion_fwrite_fs* recv_peticion_f_write_fs(int socket) {
	t_list* paquete = recibir_paquete(socket);
	t_peticion_fwrite_fs* peticion = malloc(sizeof(t_peticion_fwrite_fs));

	peticion->nombre_archivo = list_get(paquete,0);

	int* direccion_fisica = list_get(paquete, 1);
	peticion->direccion_fisica = *direccion_fisica;

	uint32_t* puntero = list_get(paquete, 2);
	peticion->puntero = *puntero;

	free(direccion_fisica);
	free(puntero);

	return peticion;
}

t_peticion_fread* recv_peticion_f_read(int socket){
	t_list* paquete = recibir_paquete(socket);
	t_peticion_fread* peticion = malloc(sizeof(t_peticion_fread));

	peticion->nombre_archivo = list_get(paquete,0);

	int* direccion_fisica = list_get(paquete, 1);
	peticion->direccion_fisica = *direccion_fisica;


	free(direccion_fisica);
	return peticion;
}
t_peticion_fread_fs* recv_peticion_f_read_fs(int socket) {
	t_list* paquete = recibir_paquete(socket);
	t_peticion_fread_fs* peticion = malloc(sizeof(t_peticion_fread_fs));

	peticion->nombre_archivo = list_get(paquete,0);

	int* direccion_fisica = list_get(paquete, 1);
	peticion->direccion_fisica = *direccion_fisica;

	uint32_t* puntero = list_get(paquete, 2);
	peticion->puntero = *puntero;

	free(direccion_fisica);
	free(puntero);

	return peticion;
}




t_peticion* recv_peticion(int socket){
	t_list* paquete = recibir_paquete(socket);
//	t_pcb* pcb = malloc(sizeof(t_pcb));
	t_peticion* peticion = malloc(sizeof(t_peticion));

	t_pcb* pcb = list_get(paquete, 0);
	free(pcb);

	char* nombre_archivo = list_get(paquete, 1);
	peticion->nombre_archivo = malloc(strlen(nombre_archivo));
	strcpy(peticion->nombre_archivo, nombre_archivo);
	free(nombre_archivo);

	char* modo_apertura = list_get(paquete, 2);
	peticion->modo_apertura = malloc(strlen(modo_apertura));
	strcpy(peticion->modo_apertura, modo_apertura);
	free(modo_apertura);

	int* direccion_fisica = list_get(paquete, 3);
	peticion->direccion_fisica = *direccion_fisica;
	free(direccion_fisica);

	uint32_t* tamanio = list_get(paquete, 4);
	peticion->tamanio = *tamanio;
	free(tamanio);

	uint32_t* posicion = list_get(paquete, 5);
	peticion->posicion = *posicion;
	free(posicion);

	list_destroy(paquete);
	return peticion;
}
void send_bloques_reservados(int socket, t_list* lista_bloques_reservados, int tamanio){
	t_paquete* paquete = crear_paquete(SOLICITUD_BLOQUES_SWAP);
	agregar_a_paquete(paquete, &lista_bloques_reservados, tamanio);
	enviar_paquete(paquete, socket);

	eliminar_paquete(paquete);
}

uint32_t* recv_lista_bloques_reservados(int socket){
	t_list* paquete = recibir_paquete(socket);
	uint32_t* lista_bloques_reservados = list_get(paquete, 0);

	list_destroy(paquete);
	return lista_bloques_reservados;
}

char* recv_parametros_fopen(int socket){
	t_list* paquete = recibir_paquete(socket);
	char* nombre_archivo = list_get(paquete, 0);

	return nombre_archivo;
}

t_list* recv_parametros(int socket){
	t_list* paquete = recibir_paquete(socket);
	t_list* parametros = paquete;

	return parametros;
}


void send_finalizo_fopen(int socket, int numero){
	t_paquete* paquete = crear_paquete(FIN_FOPEN);
	agregar_a_paquete(paquete, &numero, sizeof(numero));
	enviar_paquete(paquete, socket);
	eliminar_paquete(paquete);
}

int recv_finalizo_fopen(int socket){
	t_list* paquete = recibir_paquete(socket);
	int* numero_recibido = list_get(paquete, 0);
	int numero = *numero_recibido;

	free(numero_recibido);
	list_destroy(paquete);
	return numero;
}

void send_finalizo_ftruncate(int socket, int numero){
	t_paquete* paquete = crear_paquete(FIN_FTRUNCATE);
	agregar_a_paquete(paquete, &numero, sizeof(numero));
	enviar_paquete(paquete, socket);
	eliminar_paquete(paquete);
}

int recv_finalizo_ftruncate(int socket){
	t_list* paquete = recibir_paquete(socket);
	int* numero_recibido = list_get(paquete, 0);
	int numero = *numero_recibido;

	free(numero_recibido);
	list_destroy(paquete);
	return numero;
}

void send_finalizo_fread(int socket, int numero){
	t_paquete* paquete = crear_paquete(FIN_FREAD);
	agregar_a_paquete(paquete, &numero, sizeof(numero));
	enviar_paquete(paquete, socket);
	eliminar_paquete(paquete);
}

int recv_finalizo_fread(int socket){
	t_list* paquete = recibir_paquete(socket);
	int* numero_recibido = list_get(paquete, 0);
	int numero = *numero_recibido;

	free(numero_recibido);
	list_destroy(paquete);
	return numero;
}

void send_finalizo_fwrite(int socket, int numero){
	t_paquete* paquete = crear_paquete(FIN_FWRITE);
	agregar_a_paquete(paquete, &numero, sizeof(numero));
	enviar_paquete(paquete, socket);
	eliminar_paquete(paquete);
}

int recv_finalizo_fwrite(int socket){
	t_list* paquete = recibir_paquete(socket);
	int* numero_recibido = list_get(paquete, 0);
	int numero = *numero_recibido;

	free(numero_recibido);
	list_destroy(paquete);
	return numero;
}


//SOLICITUD DE LECTURA DE MEMORIA
void send_solicitud_lectura(int direccion_fisica, int fd_memoria){
	t_paquete* paquete = crear_paquete(LEER_MEMORIA);

	agregar_a_paquete(paquete, &direccion_fisica, sizeof(int));

	enviar_paquete(paquete, fd_memoria);
	eliminar_paquete(paquete);
}

int recv_solicitud_lectura(int fd_cpu){
	t_list* paquete = recibir_paquete(fd_cpu);
	int* direccion_fisica = list_get(paquete, 0);

	list_destroy(paquete);
	return *direccion_fisica;
}

uint32_t* recv_valor_leido(int fd_memoria){
	t_list* paquete = recibir_paquete(fd_memoria);
	uint32_t* valor = list_get(paquete, 0);

	list_destroy(paquete);
	return valor;
}

void send_solicitud_valor_en_bloque(int fd_filesystem, int direccion_bloque){
	t_paquete* paquete = crear_paquete(VALOR_EN_BLOQUE);

	agregar_a_paquete(paquete, &direccion_bloque, sizeof(int));

	enviar_paquete(paquete, fd_filesystem);
	eliminar_paquete(paquete);
}

int recv_solicitud_valor_en_bloque(int fd_memoria){
	t_list* paquete = recibir_paquete(fd_memoria);
	int* direccion_bloque = list_get(paquete, 0);

	list_destroy(paquete);
	return *direccion_bloque;
}

void send_valor_en_bloque(int fd_memoria, uint32_t valor){
	t_paquete* paquete = crear_paquete(VALOR_EN_BLOQUE);

	agregar_a_paquete(paquete, &valor, sizeof(uint32_t));

	enviar_paquete(paquete, fd_memoria);
	eliminar_paquete(paquete);
}

uint32_t recv_valor_en_bloque(int fd_filesystem){
	t_list* paquete = recibir_paquete(fd_filesystem);
	uint32_t* valor = list_get(paquete, 0);

	list_destroy(paquete);
	return *valor;
}
