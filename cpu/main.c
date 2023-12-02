#include "include/main.h"

int fd_memoria = 0;
int dispatch_cliente_fd = 0;
int interrupt_cliente_fd = 0;
bool flag_hay_interrupcion = false;
t_log* logger;
t_pcb* pcb;

int main(void) {
	logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_DEBUG);
	leer_config();
	sem_init(&sem_nuevo_proceso, 0, 1);
	sem_init(&sem_ciclo_de_instrucciones, 0, 0);
	sem_init(&sem_interrupcion, 0, 0);
	fd_memoria = crear_conexion(logger, config_cpu.ip_memoria, config_cpu.puerto_memoria);
	enviar_mensaje("Hola, soy el CPU!", fd_memoria);
//	liberar_conexion(fd_memoria);

	pthread_t *hilo_dispatch = malloc(sizeof(pthread_t));
	pthread_t *hilo_interrupt = malloc(sizeof(pthread_t));

	pthread_create(hilo_dispatch, NULL, &ejecutar_pcb, NULL);
	pthread_create(hilo_interrupt, NULL, &ejecutar_interrupcion, NULL);

	pthread_join(*hilo_dispatch, NULL);
	pthread_join(*hilo_interrupt, NULL);

	return EXIT_SUCCESS;
}

void* ejecutar_pcb(void *arg) {
	int dispatch_server_fd = iniciar_servidor(config_cpu.puerto_escucha_dispatch);
	log_info(logger, "DISPATCH CPU LISTO.");
	dispatch_cliente_fd = esperar_cliente(logger, dispatch_server_fd);

	while (true) {
		sem_wait(&sem_nuevo_proceso);
		//MUTEX
		flag_hay_interrupcion = false;
		int cod_op = recibir_operacion(dispatch_cliente_fd);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(logger, dispatch_cliente_fd);
//			sem_post(&sem_nuevo_proceso);
			sem_post(&sem_interrupcion);
			break;
		case PCB:
			pcb = recv_pcb(dispatch_cliente_fd);
			ejecutar_instrucciones(pcb);
			sem_post(&sem_nuevo_proceso);
			break;
		case -1:
			log_error(logger, "El cliente se desconecto.");
			dispatch_cliente_fd = esperar_cliente(logger, dispatch_server_fd);
			break;
		default:
			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
}

void* ejecutar_interrupcion(void *arg) {
	int interrupt_server_fd = iniciar_servidor(config_cpu.puerto_escucha_interrupt);
	log_info(logger, "INTERRUPT CPU LISTO.");
	int interrupt_cliente_fd = esperar_cliente(logger, interrupt_server_fd);

	while (1) {
		sem_wait(&sem_interrupcion);
		flag_hay_interrupcion = false;
		int cod_op = recibir_operacion(interrupt_cliente_fd);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(logger, interrupt_cliente_fd);
			sem_post(&sem_nuevo_proceso);
			break;
		case INTERRUPCION:

			break;
		case -1:
			log_error(logger, "El cliente se desconecto.");
			interrupt_cliente_fd = esperar_cliente(logger, interrupt_server_fd);
			break;
		default:
			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
}

t_instruccion* recibir_instruccion(){
	t_instruccion* instruccion_recibida = malloc(sizeof(t_instruccion));
	while (true) {
		int cod_op = recibir_operacion(fd_memoria);
		switch (cod_op) {
		case PROXIMA_INSTRUCCION:
			instruccion_recibida = recv_proxima_instruccion(fd_memoria);
			break;
		}

		return instruccion_recibida;
	}
}

void ejecutar_instrucciones(t_pcb* pcb){
	while(!flag_hay_interrupcion){
		fetch(pcb);
	}
}

void fetch(t_pcb* pcb){
	t_instruccion* proxima_instruccion = solicitar_instruccion(pcb->pid, pcb->program_counter);
	log_info(logger, "PID: %d - FETCH - Program Counter: %d", pcb->pid, pcb->program_counter);
	pcb->program_counter += 1;
	decode(proxima_instruccion, pcb);
	//TODO
//	check_interrupt();
}

t_instruccion* solicitar_instruccion(int pid, int program_counter){
	t_instruccion* instruccion_recibida = malloc(sizeof(t_instruccion));
	send_solicitar_instruccion(fd_memoria, pid, program_counter);

//	int cod_op = recibir_operacion(fd_memoria);

//	instruccion_recibida = recv_proxima_instruccion(fd_memoria);

	instruccion_recibida = recibir_instruccion();

	return instruccion_recibida;
}

void decode(t_instruccion* instruccion, t_pcb* pcb){
	codigo_instruccion cod_instruccion = instruccion->codigo;

	switch(cod_instruccion){
	case SET:
		ejecutar_set(pcb, instruccion->param1, instruccion->param2);
		break;
	case SUM:
		ejecutar_sum(pcb, instruccion->param1, instruccion->param2);
		break;
	case SUB:
		ejecutar_sub(pcb, instruccion->param1, instruccion->param2);
		break;
	case JNZ:
		ejecutar_jnz(pcb, instruccion->param1, instruccion->param2);
		break;
	case SLEEP:
		flag_hay_interrupcion = true;
		ejecutar_sleep(pcb, instruccion->param1);
		break;
	case WAIT:
		flag_hay_interrupcion = true;
		ejecutar_wait(pcb, instruccion->param1);
		break;
	case SIGNAL:
		flag_hay_interrupcion = true;
		ejecutar_signal(pcb, instruccion->param1);
		break;
	case EXIT:
		ejecutar_exit(pcb);
		break;
	}
}

void ejecutar_set(t_pcb* pcb, char* param1, char* param2){
	log_info(logger, "PID: %d - Ejecutando: %s - [%s, %s]", pcb->pid, "SET", param1, param2);
	if(strcmp(param1, "AX") == 0){
		pcb->registros_generales_cpu.ax = (uint32_t)strtoul(param2, NULL, 10);
	} else if(strcmp(param1, "BX") == 0){
		pcb->registros_generales_cpu.bx = (uint32_t)strtoul(param2, NULL, 10);
	} else if(strcmp(param1, "CX") == 0){
		pcb->registros_generales_cpu.cx = (uint32_t)strtoul(param2, NULL, 10);
	} else if(strcmp(param1, "DX") == 0){
		pcb->registros_generales_cpu.dx = (uint32_t)strtoul(param2, NULL, 10);
	}
}

void ejecutar_sum(t_pcb* pcb, char* param1, char* param2){
	uint32_t parametroASumar1;// = (uint32_t)strtoul(param1, NULL, 10);
	uint32_t parametroASumar2;// = (uint32_t)strtoul(param2, NULL, 10);

	log_info(logger, "PID: %d - Ejecutando: %s - [%s, %s]", pcb->pid, "SUM", param1, param2);

	if(strcmp(param1, "AX") == 0){
			parametroASumar1 = pcb->registros_generales_cpu.ax;// = parametroASumar1 + parametroASumar2;
		} else if(strcmp(param1, "BX") == 0){
			parametroASumar1 = pcb->registros_generales_cpu.bx;// = parametroASumar1 + parametroASumar2;
		} else if(strcmp(param1, "CX") == 0){
			parametroASumar1 = pcb->registros_generales_cpu.cx;// = parametroASumar1 + parametroASumar2;
		} else if(strcmp(param1, "DX") == 0){
			parametroASumar1 = pcb->registros_generales_cpu.dx;// = parametroASumar1 + parametroASumar2;
	}

	if(strcmp(param2, "AX") == 0){
			parametroASumar2 = pcb->registros_generales_cpu.ax;// = parametroASumar1 + parametroASumar2;
		} else if(strcmp(param2, "BX") == 0){
			parametroASumar2 = pcb->registros_generales_cpu.bx;// = parametroASumar1 + parametroASumar2;
		} else if(strcmp(param2, "CX") == 0){
			parametroASumar2 = pcb->registros_generales_cpu.cx;// = parametroASumar1 + parametroASumar2;
		} else if(strcmp(param2, "DX") == 0){
			parametroASumar2 = pcb->registros_generales_cpu.dx;// = parametroASumar1 + parametroASumar2;
	}

	if(strcmp(param1, "AX") == 0){
			pcb->registros_generales_cpu.ax = parametroASumar1 + parametroASumar2;
		} else if(strcmp(param1, "BX") == 0){
			pcb->registros_generales_cpu.bx = parametroASumar1 + parametroASumar2;
		} else if(strcmp(param1, "CX") == 0){
			pcb->registros_generales_cpu.cx = parametroASumar1 + parametroASumar2;
		} else if(strcmp(param1, "DX") == 0){
			pcb->registros_generales_cpu.dx = parametroASumar1 + parametroASumar2;
	}
}

void ejecutar_sub(t_pcb* pcb, char* param1, char* param2){
	uint32_t parametroARestar1 = (uint32_t)strtoul(param1, NULL, 10);
	uint32_t parametroARestar2 = (uint32_t)strtoul(param2, NULL, 10);

	log_info(logger, "PID: %d - Ejecutando: %s - [%s, %s]", pcb->pid, "SUB", param1, param2);

	if(strcmp(param1, "AX") == 0){
			pcb->registros_generales_cpu.ax = parametroARestar1 - parametroARestar2;
		} else if(strcmp(param1, "BX") == 0){
			pcb->registros_generales_cpu.bx = parametroARestar1 - parametroARestar2;
		} else if(strcmp(param1, "CX") == 0){
			pcb->registros_generales_cpu.cx = parametroARestar1 - parametroARestar2;
		} else if(strcmp(param1, "DX") == 0){
			pcb->registros_generales_cpu.dx = parametroARestar1 - parametroARestar2;
	}
}

void ejecutar_jnz(t_pcb* pcb, char* param1, char* param2){
	int nuevo_program_counter = (int)strtoul(param2, NULL, 10);

	log_info(logger, "PID: %d - Ejecutando: %s - [%s, %s]", pcb->pid, "JNZ", param1, param2);

	if(strcmp(param1, "AX") == 0){
		if(pcb->registros_generales_cpu.ax != 0){
			pcb->program_counter = nuevo_program_counter;
		}
	} else if(strcmp(param1, "BX") == 0){
		if(pcb->registros_generales_cpu.bx != 0){
			pcb->program_counter = nuevo_program_counter;
		}
	} else if(strcmp(param1, "CX") == 0){
		if(pcb->registros_generales_cpu.cx != 0){
			pcb->program_counter = nuevo_program_counter;
		}
	} else if(strcmp(param1, "DX") == 0){
		if(pcb->registros_generales_cpu.dx != 0){
			pcb->program_counter = nuevo_program_counter;
		}
	}
}

void ejecutar_sleep(t_pcb* pcb, char* param1){
	log_info(logger, "PID: %d - Ejecutando: %s - [%s]", pcb->pid, "SLEEP", param1);
	int tiempo_bloqueado = strtoul(param1, NULL, 10);
	send_pcb(pcb, dispatch_cliente_fd);
	send_sleep(tiempo_bloqueado, dispatch_cliente_fd);
}

void ejecutar_wait(t_pcb* pcb, char* param1){
	log_info(logger, "PID: %d - Ejecutando: %s - [%s]", pcb->pid, "WAIT", param1);
	char* recurso = malloc(strlen(param1) + 1);
	strcpy(recurso, param1);
	send_pcb(pcb, dispatch_cliente_fd);
	send_recurso_wait(recurso, dispatch_cliente_fd);
	free(recurso);
}

void ejecutar_signal(t_pcb* pcb, char* param1) {
	log_info(logger, "PID: %d - Ejecutando: %s - [%s]", pcb->pid, "SIGNAL", param1);
	char* recurso = malloc(strlen(param1) + 1);
	strcpy(recurso, param1);
	send_pcb(pcb, dispatch_cliente_fd);
	send_recurso_signal(recurso, dispatch_cliente_fd);
	free(recurso);
}

void ejecutar_exit(t_pcb* pcb){
	log_info(logger, "PID: %d - Ejecutando: %s", pcb->pid, "EXIT");
	flag_hay_interrupcion = true;
	send_pcb(pcb, dispatch_cliente_fd);
	send_cambiar_estado(EXIT_ESTADO, dispatch_cliente_fd);
}

t_config* iniciar_config(void)
{
	t_config* nuevo_config = config_create("./cpu.config");

	if(nuevo_config == NULL){
		printf("No se pudo leer el config\n");
		exit(2);
	}

	return nuevo_config;
}

void leer_config(){
	t_config* config;
	config = iniciar_config();

	config_cpu.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	config_cpu.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	config_cpu.puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	config_cpu.puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
}
