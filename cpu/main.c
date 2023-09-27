#include "include/main.h"

int main(void) {
	logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_DEBUG);
	int fd_memoria = 0;
	leer_config();

	fd_memoria = crear_conexion(logger, config_cpu.ip_memoria, config_cpu.puerto_memoria);
	enviar_mensaje("Hola, soy el CPU!", fd_memoria);
//	liberar_conexion(fd_memoria);

	pthread_t* hilo_dispatch = malloc(sizeof(pthread_t));
	pthread_t* hilo_interrupt = malloc(sizeof(pthread_t));

	pthread_create(hilo_dispatch, NULL, &ejecutar_pcb, NULL);
	pthread_create(hilo_interrupt, NULL, &ejecutar_interrupcion, NULL);

	pthread_join(*hilo_dispatch, NULL);
	pthread_join(*hilo_interrupt, NULL);

	return EXIT_SUCCESS;
}

void* ejecutar_pcb(void *arg) {
	int dispatch_server_fd = iniciar_servidor(config_cpu.puerto_escucha_dispatch);
	log_info(logger, "DISPATCH CPU LISTO.");
	int dispatch_cliente_fd = esperar_cliente(dispatch_server_fd);

	while (1) {
		int cod_op = recibir_operacion(dispatch_cliente_fd);

		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(dispatch_cliente_fd);
			break;
		case PCB:
			break;
		case -1:
			log_error(logger, "El cliente se desconecto.");
			dispatch_cliente_fd = esperar_cliente(dispatch_server_fd);
			break;
		default:
			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
}

void* ejecutar_interrupcion(t_pcb* pcb, void *arg) {
	t_interrupt* interrupcion;
	int interrupt_server_fd = iniciar_servidor(config_cpu.puerto_escucha_interrupt);
	log_info(logger, "INTERRUPT CPU LISTO.");
	int cliente_fd = esperar_cliente(interrupt_server_fd);

	interrupcion = recv_interrupcion(cliente_fd);
	if(interrupcion->motivo == FIN_PROCESO) {
		ejecutar_exit(pcb);
	}

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
	case EXIT:
		ejecutar_exit(pcb);
		break;
	}
}

void ejecutar_set(t_pcb* pcb, char* param1, char* param2){
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
	uint32_t parametroASumar1 = (uint32_t)strtoul(param1, NULL, 10);
	uint32_t parametroASumar2 = (uint32_t)strtoul(param2, NULL, 10);

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

void ejecutar_exit(t_pcb* pcb){
	//TODO Cambiar estado
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
