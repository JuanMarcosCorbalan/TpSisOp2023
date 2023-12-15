#include "include/main.h"

int fd_memoria = 0;
int dispatch_cliente_fd = 0;
int interrupt_cliente_fd = 0;
bool flag_hay_interrupcion = false;
int tam_pagina = 16;
t_log* logger;
t_pcb* pcb;

int main(void) {
	logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_DEBUG);
	leer_config();
	sem_init(&sem_nuevo_proceso, 0, 1);
	sem_init(&sem_ciclo_instruccion, 0, 0);
	sem_init(&sem_interrupcion, 0, 0);
	fd_memoria = crear_conexion(logger, config_cpu.ip_memoria, config_cpu.puerto_memoria);
	enviar_mensaje("Hola, soy el CPU!", fd_memoria);
	send_handshake_cpu_memoria(fd_memoria);
	log_info(logger, "handshake con memoria socket %d", fd_memoria);
	tam_pagina = recibir_tamanio_pagina();//recv_tam_pagina(fd_memoria);
	log_info(logger, "tamanio de pagina recibido: %d de socket %d", tam_pagina, fd_memoria);
//	liberar_conexion(fd_memoria);

	pthread_t *hilo_dispatch = malloc(sizeof(pthread_t));
	pthread_t *hilo_interrupt = malloc(sizeof(pthread_t));
	pthread_t *hilo_cilo_instruccion = malloc(sizeof(pthread_t));

	pthread_create(hilo_dispatch, NULL, &ejecutar_pcb, NULL);
	pthread_create(hilo_interrupt, NULL, &ejecutar_interrupcion, NULL);
	pthread_create(hilo_cilo_instruccion, NULL, &ciclo_de_intruccion, NULL);
	pthread_join(*hilo_dispatch, NULL);
	pthread_join(*hilo_interrupt, NULL);
	pthread_join(*hilo_cilo_instruccion, NULL);

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
		case PCB:
			pcb = recv_pcb(dispatch_cliente_fd);
			sem_post(&sem_ciclo_instruccion);
			break;
		case -1:
			log_error(logger, "El cliente se desconecto.");
			dispatch_cliente_fd = esperar_cliente(logger, dispatch_server_fd);
			break;
		default:
			log_warning(logger,"Operacion desconocida.");
			break;
		}
	}
}

void* ejecutar_interrupcion(void *arg) {
	int interrupt_server_fd = iniciar_servidor(config_cpu.puerto_escucha_interrupt);
	log_info(logger, "INTERRUPT CPU LISTO.");
	int interrupt_cliente_fd = esperar_cliente(logger, interrupt_server_fd);
	while (1) {
		int cod_op = recibir_operacion(interrupt_cliente_fd);
		switch (cod_op) {
		case INTERRUPCION:
			t_motivo_exit motivo = recv_interrupcion(interrupt_cliente_fd);
			if(motivo == INTERRUPT) {
				flag_hay_interrupcion = true;
			}
			sem_wait(&sem_interrupcion);
			log_info(logger, "Interrupcion solicitada desde Kernel.");
			pcb->motivo_exit = INTERRUPT;
			send_pcb(pcb, dispatch_cliente_fd);
			send_cambiar_estado(EXIT_ESTADO, dispatch_cliente_fd);
			sem_post(&sem_nuevo_proceso);
			break;
		case -1:
			log_error(logger, "El cliente se desconecto.");
			interrupt_cliente_fd = esperar_cliente(logger, interrupt_server_fd);
			break;
		default:
			log_warning(logger,"Operacion desconocida.");
			break;
		}
	}
}

void* ciclo_de_intruccion(void *arg){
	while(1){
		sem_wait(&sem_ciclo_instruccion);
		fetch(pcb);
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
}

t_instruccion* solicitar_instruccion(int pid, int program_counter){
	t_instruccion* instruccion_recibida = malloc(sizeof(t_instruccion));
	send_solicitar_instruccion(fd_memoria, pid, program_counter);
	instruccion_recibida = recibir_instruccion();

	return instruccion_recibida;
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

void decode(t_instruccion* instruccion, t_pcb* pcb){
	codigo_instruccion cod_instruccion = instruccion->codigo;

	switch(cod_instruccion){
	case SET:
		ejecutar_set(pcb, instruccion->param1, instruccion->param2);
		check_interrupt();
		break;
	case SUM:
		ejecutar_sum(pcb, instruccion->param1, instruccion->param2);
		check_interrupt();
		break;
	case SUB:
		ejecutar_sub(pcb, instruccion->param1, instruccion->param2);
		check_interrupt();
		break;
	case JNZ:
		ejecutar_jnz(pcb, instruccion->param1, instruccion->param2);
		check_interrupt();
		break;
	case SLEEP:
		ejecutar_sleep(pcb, instruccion->param1);
		break;
	case WAIT:
		ejecutar_wait(pcb, instruccion->param1);
		break;
	case SIGNAL:
		ejecutar_signal(pcb, instruccion->param1);
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
	case F_OPEN:
		ejecutar_fopen(pcb, instruccion->param1, instruccion->param2);
		break;
	case F_CLOSE:
		ejecutar_fclose(pcb, instruccion->param1);
		break;
	case F_SEEK:
		ejecutar_fseek(pcb, instruccion->param1, instruccion->param2);
		break;
	case F_WRITE:
		ejecutar_fwrite(pcb, instruccion->param1, instruccion->param2);
		break;
	case F_READ:
		ejecutar_fread(pcb, instruccion->param1, instruccion->param2);
		break;
	case F_TRUNCATE:
		ejecutar_ftruncate(pcb, instruccion->param1, instruccion->param2);
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
	uint32_t parametroASumar1;
	uint32_t parametroASumar2;

	log_info(logger, "PID: %d - Ejecutando: %s - [%s, %s]", pcb->pid, "SUM", param1, param2);

	if(strcmp(param1, "AX") == 0){
			parametroASumar1 = pcb->registros_generales_cpu.ax;
		} else if(strcmp(param1, "BX") == 0){
			parametroASumar1 = pcb->registros_generales_cpu.bx;
		} else if(strcmp(param1, "CX") == 0){
			parametroASumar1 = pcb->registros_generales_cpu.cx;
		} else if(strcmp(param1, "DX") == 0){
			parametroASumar1 = pcb->registros_generales_cpu.dx;
	}

	if(strcmp(param2, "AX") == 0){
			parametroASumar2 = pcb->registros_generales_cpu.ax;
		} else if(strcmp(param2, "BX") == 0){
			parametroASumar2 = pcb->registros_generales_cpu.bx;
		} else if(strcmp(param2, "CX") == 0){
			parametroASumar2 = pcb->registros_generales_cpu.cx;
		} else if(strcmp(param2, "DX") == 0){
			parametroASumar2 = pcb->registros_generales_cpu.dx;
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
	sem_post(&sem_nuevo_proceso);
}

void ejecutar_wait(t_pcb* pcb, char* param1){
	log_info(logger, "PID: %d - Ejecutando: %s - [%s]", pcb->pid, "WAIT", param1);
	char* recurso = malloc(strlen(param1) + 1);
	strcpy(recurso, param1);
	send_pcb(pcb, dispatch_cliente_fd);
	send_recurso_wait(recurso, dispatch_cliente_fd);
	free(recurso);
	sem_post(&sem_nuevo_proceso);
}

void ejecutar_signal(t_pcb* pcb, char* param1) {
	log_info(logger, "PID: %d - Ejecutando: %s - [%s]", pcb->pid, "SIGNAL", param1);
	char* recurso = malloc(strlen(param1) + 1);
	strcpy(recurso, param1);
	send_pcb(pcb, dispatch_cliente_fd);
	send_recurso_signal(recurso, dispatch_cliente_fd);
	free(recurso);
	sem_post(&sem_nuevo_proceso);
}

void ejecutar_exit(t_pcb* pcb){
	log_info(logger, "PID: %d - Ejecutando: %s", pcb->pid, "EXIT");
	send_pcb(pcb, dispatch_cliente_fd);
	send_cambiar_estado(EXIT_ESTADO, dispatch_cliente_fd);
	sem_post(&sem_nuevo_proceso);
}

void check_interrupt(){
	if(flag_hay_interrupcion) {
		sem_post(&sem_interrupcion);
	} else {
		sem_post(&sem_ciclo_instruccion);
	}
}

int recibir_marco(){
	int marco;
	while (true) {
		int cod_op = recibir_operacion(fd_memoria);
		switch (cod_op) {
		case MARCO:
			marco = recv_marco(fd_memoria);
			break;
		}

		return marco;
	}
}

int solicitar_direccion_fisica(t_pcb* pcb, int direccion_logica){
	int numero_pagina = floor(direccion_logica / tam_pagina);
	int desplazamiento =  direccion_logica - numero_pagina * tam_pagina;
	send_solicitud_marco(fd_memoria, pcb->pid, numero_pagina);
	int marco = recibir_marco();
	if(marco == -1){
		log_info(logger, "Page Fault PID: %d - Pagina: %d", pcb->pid, numero_pagina); //log ob
		//iniciar acciones page fault
		send_pcb(pcb, dispatch_cliente_fd);
		send_pcb_pf(numero_pagina, desplazamiento, dispatch_cliente_fd);
		sem_post(&sem_nuevo_proceso);
		return marco;
	}
	log_info(logger,  "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", pcb->pid, numero_pagina, marco); //log ob
	int direccion_fisica = marco*tam_pagina + desplazamiento;
	return direccion_fisica;
}

uint32_t recibir_valor_leido(){
	uint32_t valor;
	while (true) {
		int cod_op = recibir_operacion(fd_memoria);
		switch (cod_op) {
		case VALOR_LEIDO:
			valor = recv_valor_leido_memoria(fd_memoria);
			break;
		}

		return valor;
	}
}

void ejecutar_mov_in(t_pcb* pcb, char* param1, char* param2){
	log_info(logger, "PID: %d - Ejecutando: %s - [%s, %s]", pcb->pid, "MOV IN", param1, param2);
	int direccion_logica = atoi(param2);
	int direccion_fisica = solicitar_direccion_fisica(pcb, direccion_logica);
	if(direccion_fisica == -1){
		//pcb->program_counter -= 1;
		return;
	}

	send_solicitud_lectura_memoria(direccion_fisica, fd_memoria);
	uint32_t valor = recibir_valor_leido();
	log_info(logger, "PID: %d - Accion: LEER - Direccion Fisica: %d - Valor: %d", pcb->pid, direccion_fisica, valor);
	cambiar_valor_registro(pcb, param1, valor);
	check_interrupt();
}

void ejecutar_mov_out(t_pcb* pcb, char* param1, char* param2){
	log_info(logger, "PID: %d - Ejecutando: %s - [%s, %s]", pcb->pid, "MOV OUT", param1, param2);
	char* registro = param2;
	int direccion_logica = atoi(param1);
	int direccion_fisica = solicitar_direccion_fisica(pcb, direccion_logica);
	if(direccion_fisica == -1){
		//pcb->program_counter -= 1;
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

	int numero_pagina = floor(direccion_logica / tam_pagina);
	pid_y_numpag* pyn = malloc(sizeof(pid_y_numpag));
	pyn->numero_pagina = numero_pagina;
	pyn->pid = pcb->pid;
	send_solicitud_escritura_memoria(direccion_fisica, valor, pyn, fd_memoria);
	log_info(logger, "PID: %d - Accion: ESCRIBIR - Direccion Fisica: %d - Valor: %d", pcb->pid, direccion_fisica, valor);
	check_interrupt();
	free(pyn);
}

// envia a kernel el nombre del archivo y el modo de apertura W, R
void ejecutar_fopen(t_pcb* pcb, char* param1, char* param2){
	t_peticion* peticion = malloc(sizeof(t_peticion));
	peticion->nombre_archivo = strdup(param1);
	peticion->modo_apertura = strdup(param2);
	send_pcb(pcb, dispatch_cliente_fd);
	send_peticion(dispatch_cliente_fd, pcb, peticion, FOPEN);

    free(peticion->nombre_archivo);
    free(peticion->modo_apertura);
    free(peticion);

    sem_post(&sem_nuevo_proceso);
}

// solicita el cierre del archivo con el nombre del mismo
void ejecutar_fclose(t_pcb* pcb, char* param1){
	t_peticion* peticion = malloc(sizeof(t_peticion));
	peticion->nombre_archivo = strdup(param1);
	peticion->modo_apertura = "R";
	send_pcb(pcb, dispatch_cliente_fd);
	send_peticion_f_close(dispatch_cliente_fd, pcb, peticion, FCLOSE);

    free(peticion->nombre_archivo);
    free(peticion);

    sem_post(&sem_nuevo_proceso);
}

// solicita al kernel actualizar el puntero del archivo a la posición pasada por parámetro
void ejecutar_fseek(t_pcb* pcb,char* param1, char* param2){
	t_peticion* peticion = malloc(sizeof(t_peticion));
	peticion->nombre_archivo = param1;
	peticion->posicion = (uint32_t)strtoul(param2, NULL, 10);
	send_peticion(dispatch_cliente_fd, pcb, peticion, FSEEK);

    free(peticion->nombre_archivo);
    free(peticion);
}

//  solicita al Kernel que se lea del archivo indicado y se escriba en la dirección física de Memoria la información leída.
void ejecutar_fread(t_pcb* pcb, char* param1, char* param2){
	t_peticion* peticion = malloc(sizeof(t_peticion));
	peticion->nombre_archivo = param1;
//	peticion->direccion_fisica = solicitar_direccion_fisica(pcb, atoi(param2));
	send_peticion(dispatch_cliente_fd, pcb, peticion, FREAD);

	free(peticion->nombre_archivo);
    free(peticion);
}

//  solicita al Kernel que se escriba en el archivo indicado la información que es obtenida a partir de la dirección física de Memoria.
void ejecutar_fwrite(t_pcb* pcb, char* param1, char* param2){
	t_peticion* peticion = malloc(sizeof(t_peticion));
	peticion->nombre_archivo = param1;
//	peticion->direccion_fisica = solicitar_direccion_fisica(pcb, atoi(param2));
	send_peticion(dispatch_cliente_fd, pcb, peticion, FWRITE);

	free(peticion->nombre_archivo);
    free(peticion);
}

// solicita al Kernel que se modifique el tamaño del archivo al indicado por parámetro.
void ejecutar_ftruncate(t_pcb* pcb, char* param1, char* param2){
	t_peticion* peticion = malloc(sizeof(t_peticion));
	peticion->nombre_archivo = param1;
	peticion->tamanio = (uint32_t)strtoul(param2, NULL, 10);
	send_peticion(dispatch_cliente_fd, pcb, peticion, FTRUNCATE);

	free(peticion->nombre_archivo);
    free(peticion);
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

int recibir_tamanio_pagina(){
	int tamanio;
	while (true) {
		int cod_op = recibir_operacion(fd_memoria);
		switch (cod_op) {
		case TAMANIO_PAGINA:
			tamanio = recv_tam_pagina(fd_memoria);
			break;
		}

		return tamanio;
	}
}
