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

/////////////////

void send_interrupcion(t_interrupt* interrupcion, int fd){
	t_paquete* datos_interrupcion = crear_paquete(INTERRUPCION);
	t_interrupt* interrupcion_a_enviar = malloc(sizeof(t_interrupt));

	interrupcion_a_enviar->motivo = interrupcion->motivo;
	interrupcion_a_enviar->interrupt_id = interrupcion->interrupt_id;
	interrupcion_a_enviar->flag = interrupcion->flag;

	agregar_a_paquete(datos_interrupcion, interrupcion_a_enviar, sizeof(interrupcion_a_enviar));


	enviar_paquete(datos_interrupcion, fd);
	eliminar_paquete(datos_interrupcion);
}
t_interrupt* recv_interrupcion(int fd){
	t_list* paquete = recibir_paquete(fd);
	t_interrupt* interrupcion = malloc(sizeof(t_interrupt));
//	interrupcion->motivo = list_get(paquete, 0);
//	interrupcion->interrupt_id = list_get(paquete, 1);
//	interrupcion->flag = list_get(paquete, 2);
	list_destroy(paquete);
	return interrupcion;
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

	enviar_paquete(paquete, socket);
	eliminar_paquete(paquete);
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

//SEND_RECURSO_WAIT

void send_recurso_wait(int dispatch_cliente_fd, char* recurso){
	t_paquete* paquete = crear_paquete(RECURSO_WAIT);
	agregar_a_paquete(paquete, &recurso, sizeof(t_pcb));
	enviar_paquete(paquete, dispatch_cliente_fd);
	eliminar_paquete(paquete);
}

char* recv_recurso_wait(int dispatch_cliente_fd){
	t_list* paquete = recibir_paquete(dispatch_cliente_fd);
	char* recurso = list_get(paquete, 0);

	list_destroy(paquete);

	return recurso;
}

// SEND_PETICION_FS

//void send_fopen(int socket, t_pcb* pcb ,t_peticion* peticion){
//	t_paquete* paquete = crear_paquete(FOPEN);
//	agregar_a_paquete(paquete, &pcb, sizeof(t_pcb));
//	agregar_a_paquete(paquete, &(peticion->nombre_archivo), strlen(peticion->nombre_archivo) + 1);
//	agregar_a_paquete(paquete, &(peticion->modo_apertura), sizeof(char));
//	enviar_paquete(paquete, socket);
//
//	eliminar_paquete(paquete);
//}
//
//void send_fclose(int socket, t_pcb* pcb ,t_peticion* peticion){
//	t_paquete* paquete = crear_paquete(FCLOSE);
//	agregar_a_paquete(paquete, &pcb, sizeof(t_pcb));
//	agregar_a_paquete(paquete, &(peticion->nombre_archivo), strlen(peticion->nombre_archivo) + 1);
//	enviar_paquete(paquete, socket);
//
//	eliminar_paquete(paquete);
//}
//
//void send_fseek(int socket, t_pcb* pcb ,t_peticion* peticion){
//	t_paquete* paquete = crear_paquete(FSEEK);
//	agregar_a_paquete(paquete, &pcb, sizeof(t_pcb));
//	agregar_a_paquete(paquete, &(peticion->nombre_archivo), strlen(peticion->nombre_archivo) + 1);
//	agregar_a_paquete(paquete, &(peticion->posicion), sizeof(uint32_t));
//	enviar_paquete(paquete, socket);
//
//	eliminar_paquete(paquete);
//}
//
//void send_ftruncate(int socket, t_pcb* pcb ,t_peticion* peticion){
//	t_paquete* paquete = crear_paquete(FTRUNCATE);
//	agregar_a_paquete(paquete, &pcb, sizeof(t_pcb));
//	agregar_a_paquete(paquete, &(peticion->nombre_archivo), strlen(peticion->nombre_archivo) + 1);
//	agregar_a_paquete(paquete, &(peticion->tamanio), sizeof(uint32_t));
//	enviar_paquete(paquete, socket);
//
//	eliminar_paquete(paquete);
//}
//
//void send_fread(int socket, t_pcb* pcb ,t_peticion* peticion){
//	t_paquete* paquete = crear_paquete(FREAD);
//	agregar_a_paquete(paquete, &pcb, sizeof(t_pcb));
//	agregar_a_paquete(paquete, &(peticion->nombre_archivo), strlen(peticion->nombre_archivo) + 1);
//	agregar_a_paquete(paquete, &(peticion->direccion_logica), sizeof(int));
//	enviar_paquete(paquete, socket);
//
//	eliminar_paquete(paquete);
//}
//
//void send_fwrite(int socket, t_pcb* pcb ,t_peticion* peticion){
//	t_paquete* paquete = crear_paquete(FWRITE);
//	agregar_a_paquete(paquete, &pcb, sizeof(t_pcb));
//	agregar_a_paquete(paquete, &(peticion->nombre_archivo), strlen(peticion->nombre_archivo) + 1);
//	agregar_a_paquete(paquete, &(peticion->direccion_logica), sizeof(char));
//	enviar_paquete(paquete, socket);
//
//	eliminar_paquete(paquete);
//}

void send_peticion(int socket, t_pcb* pcb ,t_peticion* peticion){
	t_paquete* paquete = crear_paquete(PETICION);
	agregar_a_paquete(paquete, &pcb, sizeof(t_pcb));
	agregar_a_paquete(paquete, &(peticion->nombre_archivo), strlen(peticion->nombre_archivo) + 1);
	agregar_a_paquete(paquete, &(peticion->modo_apertura), sizeof(char));
	agregar_a_paquete(paquete, &(peticion->direccion_logica), sizeof(char));
	agregar_a_paquete(paquete, &(peticion->tamanio), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(peticion->posicion), sizeof(uint32_t));

	enviar_paquete(paquete, socket);

	eliminar_paquete(paquete);
}

t_peticion* recv_peticion(int socket){
	t_list* paquete = recibir_paquete(socket);
//	t_pcb* pcb = malloc(sizeof(t_pcb));
	t_peticion* peticion = malloc(sizeof(t_peticion));

//	int* pid = list_get(paquete, 0);
//	pcb->pid = *pid;
//	free(pid);

	char** nombre_archivo = list_get(paquete, 1);
	peticion->nombre_archivo = *nombre_archivo;
	free(nombre_archivo);

	char* modo_apertura = list_get(paquete, 2);
	peticion->modo_apertura = *modo_apertura;
	free(modo_apertura);

	int* direccion_logica = list_get(paquete, 3);
	peticion->direccion_logica = *direccion_logica;
	free(direccion_logica);

	uint32_t* tamanio = list_get(paquete, 4);
	peticion->tamanio = *tamanio;
	free(tamanio);

	uint32_t* posicion = list_get(paquete, 5);
	peticion->posicion = *posicion;
	free(posicion);

	list_destroy(paquete);
	return peticion;
}
void send_bloques_reservados(int socket, uint32_t* lista_bloques_reservados, int tamanio){
	t_paquete* paquete = crear_paquete(INICIARPROCESO);
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

