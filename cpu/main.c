#include "include/main.h"

int fd_memoria = 0;
int dispatch_cliente_fd = 0;
bool flag_hay_interrupcion = false;
int tam_pagina = 0;
t_log* logger;
t_pcb* pcb;

int main(void) {
	logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_DEBUG);
	leer_config();
	sem_init(&sem_nuevo_proceso, 0, 1);
	sem_init(&sem_ciclo_de_instrucciones, 0, 0);
	fd_memoria = crear_conexion(logger, config_cpu.ip_memoria, config_cpu.puerto_memoria);
	send_handshake_cpu_memoria(fd_memoria, 1);
	log_info(logger, "handshake con memoria socket %d", fd_memoria);
	tam_pagina = recv_tam_pagina(fd_memoria);
	log_info(logger, "tamanio de pagina recibido: %d de socket %d", tam_pagina, fd_memoria);
//	liberar_conexion(fd_memoria);

	pthread_t *hilo_dispatch = malloc(sizeof(pthread_t));
	pthread_t *hilo_interrupt = malloc(sizeof(pthread_t));

	pthread_create(hilo_dispatch, NULL, &ejecutar_pcb, NULL);
	//pthread_create(hilo_interrupt, NULL, &ejecutar_interrupcion, NULL);

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
			sem_post(&sem_nuevo_proceso);
			break;
		case PCB:
			pcb = recv_pcb(dispatch_cliente_fd);
			//Logger

//			send_solicitar_instruccion(fd_memoria, 1, 1);

//			sem_post(&sem_nuevo_proceso);

			ejecutar_instrucciones(pcb);
//			sem_post(&sem_ciclo_de_instrucciones);
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

void* ejecutar_interrupcion(t_pcb* pcb, void *arg) {
	t_interrupt* interrupcion;
	int interrupt_server_fd = iniciar_servidor(config_cpu.puerto_escucha_interrupt);
	log_info(logger, "INTERRUPT CPU LISTO.");
//	int cliente_fd = esperar_cliente(interrupt_server_fd);

	//FALTA SEMAFORO O WHILE O ALGO
//	interrupcion = recv_interrupcion(cliente_fd);
//	if(interrupcion->motivo == END_PROCESO) {
//		ejecutar_exit(pcb);
//	}

}

void fetch(t_pcb* pcb){
	t_instruccion* proxima_instruccion = solicitar_instruccion(pcb->pid, pcb->program_counter);
	decode(proxima_instruccion, pcb);
	pcb->program_counter += 1;
	//TODO
//	check_interrupt();
}

t_instruccion* solicitar_instruccion(int pid, int program_counter){
	send_solicitar_instruccion(fd_memoria, pid, program_counter);
	log_info(logger, "Solicitud de instruccion enviada");
//	int cod_op = recibir_operacion(fd_memoria);

	t_instruccion* instruccion_recibida = recv_proxima_instruccion(fd_memoria);
	log_info(logger, "Instruccion recibida");
	//instruccion_recibida = recibir_instruccion();

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
	case WAIT:
		ejecutar_wait(pcb, instruccion->param1);
		break;
	case EXIT:
		ejecutar_exit(pcb);
		break;
	case MOV_IN:
		ejecutar_mov_in(pcb, instruccion->param1, instruccion->param2);
		break;
	case MOV_OUT:
		ejecutar_mov_out(pcb, instruccion->param1, instruccion->param2);
		break;
	}
}

void cambiar_valor_registro(t_pcb* pcb, char* registro, uint32_t nuevo_valor){
	if(strcmp(registro, "AX") == 0){
		pcb->registros_generales_cpu.ax = nuevo_valor;
	} else if(strcmp(registro, "BX") == 0){
		pcb->registros_generales_cpu.bx = nuevo_valor;
	} else if(strcmp(registro, "CX") == 0){
		pcb->registros_generales_cpu.cx = nuevo_valor;
	} else if(strcmp(registro, "DX") == 0){
		pcb->registros_generales_cpu.dx = nuevo_valor;
	}
}

void ejecutar_set(t_pcb* pcb, char* param1, char* param2){
	cambiar_valor_registro(pcb, param1, (uint32_t)strtoul(param2, NULL, 10));
}

void ejecutar_sum(t_pcb* pcb, char* param1, char* param2){
	uint32_t parametroASumar1;// = (uint32_t)strtoul(param1, NULL, 10);
	uint32_t parametroASumar2;// = (uint32_t)strtoul(param2, NULL, 10);

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

	cambiar_valor_registro(pcb, param1, parametroASumar1 + parametroASumar2);
}

void ejecutar_sub(t_pcb* pcb, char* param1, char* param2){
	uint32_t parametroARestar1 = (uint32_t)strtoul(param1, NULL, 10);
	uint32_t parametroARestar2 = (uint32_t)strtoul(param2, NULL, 10);

	cambiar_valor_registro(pcb, param1, parametroARestar1 + parametroARestar2);
}

void ejecutar_wait(t_pcb* pcb, char* param1){
	char* recurso = malloc(strlen(param1) + 1);
	strcpy(recurso, param1);
	send_pcb_actualizado(dispatch_cliente_fd, pcb);
	send_recurso_wait(dispatch_cliente_fd, recurso);
	free(recurso);
}

void ejecutar_exit(t_pcb* pcb){
	flag_hay_interrupcion = true;
	pcb->estado = SUCCESS;
	send_pcb(pcb, dispatch_cliente_fd);
}

int solicitar_direccion_fisica(int direccion_logica, int pid){
	int numero_pagina = floor(direccion_logica / tam_pagina);
	int desplazamiento =  direccion_logica - numero_pagina * tam_pagina;
	send_solicitud_marco(fd_memoria, pid, numero_pagina);
	int marco = recv_marco(fd_memoria);
	if(marco == -1){
		log_info(logger, "Page Fault PID: %d - Pagina: %d", pid, numero_pagina); //log ob
		//iniciar acciones page fault
		send_pcb_pf(pcb, numero_pagina, dispatch_cliente_fd);
		//TODO mandar interrupcion para que no actualice el program counter
		return marco;
	}
	log_info(logger,  "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", pid, numero_pagina, marco); //log ob
	return marco+desplazamiento;
}

void ejecutar_mov_in(t_pcb* pcb, char* param1, char* param2){
	int direccion_logica = atoi(param2);
	int direccion_fisica = solicitar_direccion_fisica(direccion_logica, pcb->pid);
	if(direccion_fisica == -1){
		return;
	}

	send_solicitud_lectura(direccion_fisica, fd_memoria);
	uint32_t valor = recv_valor_leido(fd_memoria);

	cambiar_valor_registro(pcb, param1, valor);
}

void ejecutar_mov_out(t_pcb* pcb, char* param1, char* param2){
	char* registro = param2;
	int direccion_logica = atoi(param1);
	int direccion_fisica = solicitar_direccion_fisica(direccion_logica, pcb->pid);
	if(direccion_fisica == -1){
		return;
	}

	uint32_t valor;

	if(strcmp(registro, "AX") == 0){
		valor = pcb->registros_generales_cpu.ax;
	} else if(strcmp(registro, "BX") == 0){
		valor = pcb->registros_generales_cpu.bx;
	} else if(strcmp(registro, "CX") == 0){
		valor = pcb->registros_generales_cpu.cx;
	} else if(strcmp(registro, "DX") == 0){
		valor = pcb->registros_generales_cpu.dx;
	}

	send_solicitud_escritura(direccion_fisica, valor, fd_memoria);
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
