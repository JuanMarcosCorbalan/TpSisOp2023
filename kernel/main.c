#include "include/main.h"

	int fd_cpu_dispatch = 0;
	int fd_cpu_interrupt = 0;
	int fd_cpu = 0;
	int fd_filesystem = 0;
	int fd_memoria = 0;
	t_log* logger;
	t_config* config;

	bool PLANIFICACION_ACTIVA = false;

int main(void)
{
	logger = iniciar_logger();
	config = iniciar_config();

	if(!conectar_modulos(logger, config, &fd_cpu_dispatch, &fd_cpu_interrupt, &fd_filesystem, &fd_memoria)){
		terminar_programa(fd_cpu_dispatch, fd_cpu_interrupt, fd_filesystem, fd_memoria, logger, config);
		return EXIT_FAILURE;
	}

	enviar_mensaje("Hola, soy el Kernel!", fd_cpu_dispatch);
	enviar_mensaje("Hola, soy el Kernel!", fd_cpu_interrupt);
	enviar_mensaje("Hola, soy el Kernel!", fd_filesystem);
	enviar_mensaje("Hola, soy el Kernel!", fd_memoria);

	inicializar_variables();

	iniciar_consola(logger, config, fd_memoria);
	terminar_programa(fd_cpu_dispatch, fd_cpu_interrupt, fd_filesystem, fd_memoria, logger, config);

	return EXIT_SUCCESS;
}

void iniciar_consola(t_log* logger, t_config* config, int fd_memoria){
	bool salir = false;

	while(!salir){
		char* entrada = readline("> ");
		add_history(entrada);

		char** argumentos_entrada = string_split(entrada, " ");

		if(string_equals_ignore_case(entrada, "SALIR")){
			salir = true;
		} else
		if(string_equals_ignore_case(argumentos_entrada[0], "INICIAR_PROCESO")){
			iniciar_proceso(logger, argumentos_entrada, fd_memoria);
		} else
		if(string_equals_ignore_case(argumentos_entrada[0], "FINALIZAR_PROCESO")){
			finalizar_proceso(argumentos_entrada);
		} else
		if(string_equals_ignore_case(argumentos_entrada[0], "INICIAR_PLANIFICACION")){
			iniciar_planificacion();
		} else
		if(string_equals_ignore_case(argumentos_entrada[0], "DETENER_PLANIFICACION")){
			detener_planificacion();
		}

		free(entrada);
		free(*argumentos_entrada);
	}
	clear_history();
	rl_clear_history();
	rl_cleanup_after_signal();
}

void iniciar_planificacion() {
	log_info(logger, "INICIO DE PLANIFICACION");
	pthread_mutex_lock(&mutex_planificacion_activa);
	PLANIFICACION_ACTIVA = true;
	pthread_mutex_unlock(&mutex_planificacion_activa);
	planificador_largo_plazo();
	planificador_corto_plazo();
}

void detener_planificacion() {
	log_info(logger, "PAUSA DE PLANIFICACION");
	pthread_mutex_lock(&mutex_planificacion_activa);
	PLANIFICACION_ACTIVA = false;
	pthread_mutex_unlock(&mutex_planificacion_activa);

	semaforos_destroy();
	inicializar_semaforos();
}

void iniciar_proceso(t_log* logger, char *args[], int fd_memoria) {
	char* path = args[1];
	int size = atoi(args[2]);
	int prioridad = atoi(args[3]);
	t_pcb* nuevo_proceso = crear_pcb(prioridad);

	send_datos_proceso(path, size, nuevo_proceso->pid, fd_memoria);

	queue_push(procesos_en_new, nuevo_proceso);
	log_info(logger, "Se crea el proceso %d en NEW", nuevo_proceso->pid);
	sem_post(&sem_procesos_new);
}

void finalizar_proceso(char *args[]){
	int pid = atoi(args[1]);

	t_pcb* proceso_a_finalizar = buscar_proceso(pid);


//	char* motivo = "DESCONOCIDO";
//	if(proceso_a_finalizar->estado == EXEC){
//		// kernel envia señal de interrupcion a traves de interrupt a cpu y este tiene que devolverle
//		// a kernel el contexto de ejecucion antes de liberar memoria
//
//		t_interrupt* nueva_interrupcion = crear_interrupcion(END_PROCESO);
//		send_interrupcion(nueva_interrupcion,fd_cpu_interrupt);
//		motivo = "INVALID_ALGO";
//	} else {
//		motivo = "SUCCESS";
//	}
//	// kernel tiene que pedirle a memoria que libere el espacio que ocupa el pcb
//	// le va a pasar mediante un paquete a memoria el pid, el path y el size.
//
//	send_datos_proceso(path,size,proceso_a_finalizar->pid,fd_memoria);
//	log_info(logger, "Finaliza el proceso %d - Motivo: %s", proceso_a_finalizar->pid, motivo);
}

t_pcb* crear_pcb(int prioridad){
	t_pcb* pcb = malloc(sizeof(t_pcb));

	pcb->pid = asignador_pid;
	asignador_pid ++;
	pcb->prioridad = prioridad;
	pcb->estado = NEW;
	pcb->program_counter = 1;

	pcb->registros_generales_cpu.ax = 0;
	pcb->registros_generales_cpu.bx = 0;
	pcb->registros_generales_cpu.cx = 0;
	pcb->registros_generales_cpu.dx = 0;

	return pcb;

}
t_interrupt* crear_interrupcion(interrupt_code motivo){
	t_interrupt* interrupcion = malloc(sizeof(t_interrupt));

	interrupcion->interrupt_id = asignador_iid;
	asignador_iid ++;
	interrupcion->motivo = motivo;
	interrupcion->flag=1;

	return interrupcion;

}

void planificador_largo_plazo(){
	pthread_t hilo_ready;
	pthread_t hilo_exit;
	pthread_create(&hilo_ready, NULL, (void*) planificar_procesos_ready, NULL);
	pthread_create(&hilo_exit, NULL, (void*) procesar_respuesta_cpu, NULL);
	pthread_detach(hilo_ready);
	pthread_detach(hilo_exit);
}

void procesar_respuesta_cpu(){
	while(PLANIFICACION_ACTIVA){
		sem_wait(&sem_procesos_exit);
		int cod_op = recibir_operacion(fd_cpu_dispatch);

		switch(cod_op){
		case PCB:
			t_pcb* pcb_actualizado = recv_pcb(fd_cpu_dispatch);
			char* motivo = motivo_to_string(pcb_actualizado->estado);
			log_info(logger, "Finaliza el proceso %d - Motivo: %s", pcb_actualizado->pid, motivo);
			//TODO Terminar proceso en memoria
			pcb_destroy(pcb_actualizado);
			sem_post(&sem_multiprogramacion);
			sem_post(&sem_proceso_exec);
		break;
		}
	}
}

void planificar_procesos_ready() {
	while(PLANIFICACION_ACTIVA){
		sem_wait(&sem_procesos_new);
		t_pcb* pcb = queue_pop_con_mutex(procesos_en_new, &mutex_cola_new);
		sem_wait(&sem_multiprogramacion);
		pasar_a_ready(pcb);
		sem_post(&sem_procesos_ready);
	}
}

void pasar_a_ready(t_pcb* pcb){
	pthread_mutex_lock(&mutex_ready_list);
	cambiar_estado(pcb, READY);
	list_add(procesos_en_ready, pcb);
	log_cola_ready();
	pthread_mutex_unlock(&mutex_ready_list);
}

void planificador_corto_plazo(){
	pthread_t hilo_exec;
	pthread_create(&hilo_exec, NULL, (void*) planificar_proceso_exec, NULL);
	pthread_detach(hilo_exec);
}

void planificar_proceso_exec(){
	while(PLANIFICACION_ACTIVA){
		sem_wait(&sem_procesos_ready);
		sem_wait(&sem_proceso_exec);

		t_pcb* pcb = obtenerProximoAEjecutar();
		cambiar_estado(pcb, EXEC);
		queue_push_con_mutex(procesos_en_exec, pcb, &mutex_cola_exec);
		send_pcb(pcb, fd_cpu_dispatch);
		sem_post(&sem_procesos_exit);
	}
}

void log_cola_ready(){
	t_list* lista_logger = obtener_lista_pid(procesos_en_ready);
	char* lista = list_to_string(lista_logger);
	log_info(logger, "Cola Ready %s: [%s]", config_get_string_value(config, "ALGORITMO_PLANIFICACION"), lista);
	list_destroy(lista_logger);
	free(lista);
}

t_list* obtener_lista_pid(t_list* lista){
	t_list* lista_de_pid = list_create();
	for (int i = 0; i < list_size(lista); i++){
		t_pcb* pcb = list_get(lista, i);
		list_add(lista_de_pid, &(pcb->pid));
	}

	return lista_de_pid;
}

t_pcb* obtenerProximoAEjecutar(){
	t_pcb* pcb;
	char* algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

	if(!strcmp(algoritmo_planificacion, "FIFO")) {
		pcb = list_pop_con_mutex(procesos_en_ready, &mutex_ready_list);
		return pcb;
	}
//	else if(!strcmp(algoritmo_planificacion, "RR")){
//
////		log_info(logger, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", pcb->pid); //log obligatorio
////		return pcb;
//	}
	else if(!strcmp(algoritmo_planificacion, "PRIORIDADES")){
		list_sort(procesos_en_ready, comparar_por_prioridad);
		pcb = list_pop_con_mutex(procesos_en_ready, &mutex_ready_list);
		return pcb;
	}

	return NULL;
}

bool comparar_por_prioridad(void *pcb1, void *pcb2){
	t_pcb* p1 = (t_pcb *)pcb1;
	t_pcb* p2 = (t_pcb *)pcb2;

	return p1->prioridad < p2->prioridad;
}

void cambiar_estado(t_pcb* pcb, estado estado){
	char* nuevo_estado = strdup(estado_to_string(estado));
	char* estado_anterior = strdup(estado_to_string(pcb->estado));
	pcb->estado = estado;
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb->pid, estado_anterior, nuevo_estado);
	free(nuevo_estado);
	free(estado_anterior);
}

t_pcb* buscar_proceso(int pid) {
	t_pcb* pcb = buscar_proceso_en_queue(pid, procesos_en_new);
	t_list* procesos = list_create();

	list_add_all(procesos, procesos_en_ready);
	list_add_all(procesos, procesos_en_blocked);

	if(pcb == NULL) {
		pcb = buscar_proceso_en_queue(pid, procesos_en_exec);
		if(pcb == NULL){
			pcb = buscar_proceso_en_list(pid, procesos);
		}
	}
	return pcb;
}

t_pcb* buscar_proceso_en_list(int pid, t_list* lista) {
	t_pcb* pcb_encontrado = malloc(sizeof(t_pcb));

	for(int i = 0; i<=list_size(lista); i++){
		pcb_encontrado = list_get(lista, i);
		if(pcb_encontrado->pid == pid){
			return pcb_encontrado;
		}
	}
	return NULL;
}

t_pcb* buscar_proceso_en_queue(int pid, t_queue* queue) {
	t_pcb* pcb_encontrado = malloc(sizeof(t_pcb));

	for(int i = 0; i<=queue_size(queue); i++){
		pcb_encontrado = queue_pop(queue);
		if(pcb_encontrado->pid == pid){
			return pcb_encontrado;
		}
	}
	return NULL;
}

t_log* iniciar_logger(void)
{
	t_log* nuevo_logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);

	if(nuevo_logger == NULL){
		printf("No se pudo leer el logger\n");
		exit(1);
	}

	return nuevo_logger;
}

t_config* iniciar_config(void)
{
	t_config* nuevo_config = config_create("./kernel.config");

	if(nuevo_config == NULL){
		printf("No se pudo leer el config\n");
		exit(2);
	}

	return nuevo_config;
}

void terminar_programa(int conexion, int conexion2, int conexion3, int conexion4, t_log* logger, t_config* config)
{
	if(logger != NULL){
		log_destroy(logger);
	}

	if(config != NULL){
		config_destroy(config);
	}

	liberar_conexion(conexion);
	liberar_conexion(conexion2);
	liberar_conexion(conexion3);
	liberar_conexion(conexion4);
}

bool conectar_modulos(t_log* logger, t_config* config, int* fd_cpu_dispatch, int* fd_cpu_interrupt, int* fd_filesystem, int* fd_memoria){

	char* ip_memoria;
	char* puerto_memoria;
	char* ip_filesystem;
	char* puerto_filesystem;
	char* ip_cpu;
	char* puerto_cpu_dispatch;
	char* puerto_cpu_interrupt;

	ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
	puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");
	ip_cpu = config_get_string_value(config, "IP_CPU");
	puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
	puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");

	*fd_cpu_dispatch = crear_conexion(logger, ip_cpu, puerto_cpu_dispatch);
	*fd_cpu_interrupt = crear_conexion(logger, ip_cpu, puerto_cpu_interrupt);
	*fd_filesystem = crear_conexion(logger, ip_filesystem, puerto_filesystem);
	*fd_memoria = crear_conexion(logger, ip_memoria, puerto_memoria);

	return *fd_cpu_dispatch != 0 && *fd_cpu_interrupt != 0 && *fd_filesystem != 0 && *fd_memoria != 0;
}

void inicializar_variables() {
	asignador_pid = 1;
	procesos_en_new = queue_create();
	procesos_en_exec = queue_create();
	procesos_en_ready = list_create();
	procesos_en_blocked = list_create();
	procesos_en_exit = list_create();

	pthread_mutex_init(&mutex_cola_new, NULL);
	pthread_mutex_init(&mutex_ready_list, NULL);
	pthread_mutex_init(&mutex_cola_exec, NULL);
	pthread_mutex_init(&mutex_logger, NULL);
	pthread_mutex_init(&mutex_planificacion_activa, NULL);

	inicializar_semaforos();
}

void inicializar_semaforos() {
	int grado_multiprogramacion = atoi(config_get_string_value(config, "GRADO_MULTIPROGRAMACION_INI"));
	sem_init(&sem_multiprogramacion, 0, grado_multiprogramacion);
	sem_init(&sem_procesos_new, 0, 0);
	sem_init(&sem_procesos_ready, 0, 0);
	sem_init(&sem_proceso_exec, 0, 1);
	sem_init(&sem_procesos_exit, 0, 0);
}

void semaforos_destroy() {
	sem_close(&sem_multiprogramacion);
	sem_close(&sem_procesos_new);
	sem_close(&sem_procesos_ready);
	sem_close(&sem_proceso_exec);
	sem_close(&sem_procesos_exit);
}

void pcb_destroy(t_pcb* pcb){
	free(pcb);
}
