#include "include/main.h"

	int fd_cpu_dispatch = 0;
	int fd_cpu_interrupt = 0;
	int fd_cpu = 0;
	int fd_filesystem = 0;
	int fd_memoria = 0;
	t_config* config;

int main(void)
{
	t_log* logger;

	logger = iniciar_logger();
	config = iniciar_config();
//	int socket_servidor;
//	char* puerto_escucha;
	int grado_multiprogramacion = atoi(config_get_string_value(config, "GRADO_MULTIPROGRAMACION_INI"));
    int quantum_maximo = atoi(config_get_string_value(config, "QUANTUM"));
	sem_init(cont_multiprogramacion, 0, grado_multiprogramacion);
	sem_init(bin_proceso_new, 0, 0);
//	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	if(!conectar_modulos(logger, config, &fd_cpu_dispatch, &fd_cpu_interrupt, &fd_filesystem, &fd_memoria)){
		terminar_programa(fd_cpu_dispatch, fd_cpu_interrupt, fd_filesystem, fd_memoria, logger, config);
		return EXIT_FAILURE;
	}

	enviar_mensaje("Hola, soy el Kernel!", fd_cpu_dispatch);
	enviar_mensaje("Hola, soy el Kernel!", fd_cpu_interrupt);
	enviar_mensaje("Hola, soy el Kernel!", fd_filesystem);
	enviar_mensaje("Hola, soy el Kernel!", fd_memoria);

	inicializar_variables();

//	paquete(fd_cpu);
//	paquete(fd_filesystem);
//	paquete(fd_memoria);

//	socket_servidor = iniciar_servidor("4455");
//	while (esperar_clientes(socket_servidor));

	pthread_create(hilo_corto_plazo, NULL, (void*) planificador_corto_plazo, NULL);
	pthread_detach(*hilo_corto_plazo);

	iniciar_consola(logger, config, fd_memoria);
	terminar_programa(fd_cpu_dispatch, fd_cpu_interrupt, fd_filesystem, fd_memoria, logger, config);

	return EXIT_SUCCESS;
}

void iniciar_consola(t_log* logger, t_config* config, int fd_memoria){
	char* entrada;
	bool salir = false;
	char** argumentos_entrada;

	pthread_create(hilo_largo_plazo, NULL, (void*) planificador_largo_plazo, NULL);
	pthread_detach(*hilo_largo_plazo);

	while(!salir){
		entrada = readline("> ");
		add_history(entrada);
		argumentos_entrada = string_split(entrada, " ");

		if(string_equals_ignore_case(entrada, "SALIR")){
			salir = true;
			free(entrada);
			free(argumentos_entrada);
			break;
		}

		if(string_equals_ignore_case(argumentos_entrada[0], "INICIAR_PROCESO")){
			iniciar_proceso(logger, argumentos_entrada, fd_memoria);
		}
		if(string_equals_ignore_case(argumentos_entrada[0], "FINALIZAR_PROCESO")){
//			finalizar_proceso(argumentos_entrada);
		}

	}
}

void iniciar_proceso(t_log* logger, char *args[], int fd_memoria) {
	char* path = args[1];
	int size = atoi(args[2]);
	int prioridad = atoi(args[3]);
	t_pcb* nuevo_proceso = crear_pcb(prioridad);

	send_datos_proceso(path, size, nuevo_proceso->pid, fd_memoria);

	queue_push(procesos_en_new, nuevo_proceso);
	log_info(logger, "Se crea el proceso %d en NEW", nuevo_proceso->pid);
	sem_post(bin_proceso_new);
}

void finalizar_proceso(t_log* logger, t_pcb* proceso_a_finalizar,char *args[], int fd_memoria, int fd_cpu_interrupt){
	char* path = args[1];
	int size = atoi(args[2]);
	char* motivo = "DESCONOCIDO";
	if(proceso_a_finalizar->estado == EXEC){
		// kernel envia señal de interrupcion a traves de interrupt a cpu y este tiene que devolverle
		// a kernel el contexto de ejecucion antes de liberar memoria

		t_interrupt* nueva_interrupcion = crear_interrupcion(END_PROCESO);
		send_interrupcion(nueva_interrupcion,fd_cpu_interrupt);
		motivo = "INVALID_ALGO";
	} else {
		motivo = "SUCCESS";
	}
	// kernel tiene que pedirle a memoria que libere el espacio que ocupa el pcb
	// le va a pasar mediante un paquete a memoria el pid, el path y el size.

	send_datos_proceso(path,size,proceso_a_finalizar->pid,fd_memoria);
	log_info(logger, "Finaliza el proceso %d - Motivo: <%s>", proceso_a_finalizar->pid, motivo);
}

t_pcb* crear_pcb(int prioridad){
	t_pcb* pcb = malloc(sizeof(t_pcb));

	pcb->pid = asignador_pid;
	asignador_pid ++;
	pcb->prioridad = prioridad;
	pcb->estado = NEW;
	pcb->program_counter = 1;

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
	while(true){
		sem_wait(bin_proceso_new);
		sem_wait(cont_multiprogramacion);
		t_pcb* proceso = queue_pop(procesos_en_new);
		pasar_a_ready(proceso);
	}
}

void pasar_a_ready(t_pcb* proceso){
	list_add(procesos_en_ready, proceso);
	proceso->estado = READY;
	// TODO falta enviar a memoria que ya esta ready
}

void* planificador_corto_plazo(void){
	t_pcb* pcb;
	while(1){
		//semaforo
		pcb = obtenerProximoAEjecutar();
		pcb->estado = EXEC;
		send_ejecutar_pcb(fd_cpu, pcb);

		pcb = recv_pcb_actualizado(fd_cpu);

		switch(pcb->estado){
			case(EXIT_ESTADO):
				list_add(procesos_en_exit, pcb);
				break;
			case(BLOCKED):
				list_add(procesos_en_blocked, pcb);
				break;
		}
	}
}
void* planificador_corto_plazo_RR(t_temporal* quantum){ // despues me fijo como lo integro al planificador que ya existe
	t_pcb* pcb;
	while(1){
		//semaforo
		pcb = obtenerProximoAEjecutar();
		pcb->estado = EXEC;
		send_ejecutar_pcb(fd_cpu, pcb);
		// una vez que se envia la pcb empieza a contabilizar el tiempo de quantum
		quantum = temporal_create();

		pcb = recv_pcb_actualizado(fd_cpu);
		temporal_stop(quantum); // freno el quantum
		switch(pcb->estado){
			case(EXIT_ESTADO):
				list_add(procesos_en_exit, pcb);
				break;
			case(BLOCKED):
				list_add(procesos_en_blocked, pcb);
				break;
		}
	}
}

t_pcb* obtenerProximoAEjecutar(int quantum_maximo){

	t_pcb* pcb;
	char* algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

	if(!strcmp(algoritmo_planificacion, "FIFO")) {

		pcb = list_pop_con_mutex(procesos_en_ready, mutex_ready_list);
		log_info(logger, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", pcb->pid); //log obligatorio
		return pcb;
	}
	else if(!strcmp(algoritmo_planificacion, "RR")){
		// si el quantum, se excedio, cambio el proceso y este lo mando a la cola de ready, si no sigue
		if(quantum->elapsed_ms >= quantum_maximo){
			log_info(logger, "PID: %d - Desalojado por fin de Quantum", pcb->pid); //log obligatorio
			list_push_con_mutex(procesos_en_ready, pcb,mutex_ready_list);
			pcb = list_pop_con_mutex(procesos_en_ready, mutex_ready_list);
			log_info(logger, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", pcb->pid); //log obligatorio
			return pcb;
		};

		log_info(logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXEC", pcb->pid); //log obligatorio
		// tiene que avisar si no hay cambio de estado?
		return pcb;
	}
	else if(!strcmp(algoritmo_planificacion, "PRIORIDADES")){

		log_info(logger, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", pcb->pid); //log obligatorio
		return pcb;
	}
	else{
		log_error(logger, "Error en la lectura del algoritmo de planificacion");
		exit(EXIT_FAILURE);
	}

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
	procesos_en_ready = list_create();
	procesos_en_blocked = list_create();
	procesos_en_exit = list_create();
}
