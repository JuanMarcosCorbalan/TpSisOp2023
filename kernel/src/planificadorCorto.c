#include "../include/planificadorCorto.h"

void planificadorCorto(int socket_cpu){
	t_pcb* pcb;
	t_pcb* pcb_recibido;

//	char** parametros;

	while(1){


		//sem_wait(&sem_cant_ready); //entra solo si hay algun proceso en ready, es una espera no activa
		pcb = obtenerProximoAEjecutar();

		//COMIENZA LA EJECUCION

		char** sin_parametros = string_array_new();
		//TODO
//		enviar_pcb(socket_cpu, pcb, PCB_A_EJECUTAR, sin_parametros);
		string_array_destroy(sin_parametros);
//		liberar_pcb(pcb); // TODO

		t_msj_kernel_cpu respuesta = esperar_cpu(socket_cpu);

//		parametros = recibir_parametros_de_instruccion();
		pcb_recibido = recv_pcb_cpu(socket_cpu);

		switch(respuesta){
			//case F_OPEN:
			//break;

			//case F_CLOSE:
			//break;
		}

//		string_array_destroy(parametros);
	}

}

t_pcb* obtenerProximoAEjecutar(t_list* procesos_en_ready){

	t_pcb* pcb;

	if(proximoAEjecutar){
		pcb = proximoAEjecutar;
		proximoAEjecutar = NULL;
		return pcb;
	}
	else if(!strcmp(lecturaConfig.ALGORITMO_PLANIFICACION, "FIFO")) {

		pcb = list_pop_con_mutex(procesos_en_ready, &mutex_ready_list);
//		pcb->tiempo_inicial_ejecucion = time(NULL);
		log_info(logger, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", pcb->pid); //log obligatorio
		return pcb;
	}
	else if(!strcmp(lecturaConfig.ALGORITMO_PLANIFICACION, "RR")){

		log_info(logger, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", pcb->pid); //log obligatorio
		return pcb;
	}
	else if(!strcmp(lecturaConfig.ALGORITMO_PLANIFICACION, "PRIORIDADES")){

		log_info(logger, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", pcb->pid); //log obligatorio
		return pcb;
	}
	else{
		log_error(logger, "Error en la lectura del algoritmo de planificacion");
		exit(EXIT_FAILURE);
	}

}

