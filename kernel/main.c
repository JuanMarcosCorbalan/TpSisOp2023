#include "include/main.h"

	int fd_cpu_dispatch = 0;
	int fd_cpu_interrupt = 0;
	int fd_cpu = 0;
	int fd_filesystem = 0;
	int fd_memoria = 0;
	t_log* logger;
	t_config* config;
	t_list* recursos_asignados;

	bool PLANIFICACION_ACTIVA = false;
	bool PLANIFICADOR_INICIADO = false;

int main(void)
{
	logger = iniciar_logger();
	config = iniciar_config();

	if(!conectar_modulos(logger, config, &fd_cpu_dispatch, &fd_cpu_interrupt, &fd_filesystem, &fd_memoria)){
		terminar_programa(fd_cpu_dispatch, fd_cpu_interrupt, fd_filesystem, fd_memoria, logger, config);
		return EXIT_FAILURE;
	}

	enviar_mensaje("Hola, soy el Kernel!", fd_filesystem);
	enviar_mensaje("Hola, soy el Kernel!", fd_memoria);

	recursos_asignados = list_create();
	inicializar_variables();
	tabla_global_archivos_abiertos = list_create();
	locks_encolados = list_create();

	procesos_espera_fs = list_create();

	generar_conexion_fs();

	pthread_t *hilo_consola = malloc(sizeof(pthread_t));
	pthread_create(hilo_consola, NULL, &iniciar_consola, NULL);
	pthread_join(*hilo_consola, NULL);

	terminar_programa(fd_cpu_dispatch, fd_cpu_interrupt, fd_filesystem, fd_memoria, logger, config);

	return EXIT_SUCCESS;
}

void* iniciar_consola(){
	while(1){
		char* entrada = readline("> ");
		add_history(entrada);

		char** argumentos_entrada = string_split(entrada, " ");

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
		} else
		if(string_equals_ignore_case(argumentos_entrada[0], "MULTIPROGRAMACION")){
			actualizar_multiprogramacion(argumentos_entrada);
		} else
		if(string_equals_ignore_case(argumentos_entrada[0], "PROCESO_ESTADO")){
			listar_estados_proceso();
		}

		free(entrada);
		char** args = argumentos_entrada;
		for (int i = 0; args[i] != NULL; i++) {
		    free(args[i]);
		}
		free(argumentos_entrada);
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
	if(!PLANIFICADOR_INICIADO) {
		planificador_largo_plazo();
		planificador_corto_plazo();
		PLANIFICADOR_INICIADO = true;
	}
}

void detener_planificacion() {
	log_info(logger, "PAUSA DE PLANIFICACION");
	pthread_mutex_lock(&mutex_planificacion_activa);
	PLANIFICACION_ACTIVA = false;
	pthread_mutex_unlock(&mutex_planificacion_activa);

	semaforos_destroy();
	inicializar_semaforos();
}

void actualizar_multiprogramacion(char *args[]){
	int nuevo_grado_multiprogramacion = atoi(args[1]);
	int grado_multiprogramacion_inicial = atoi(config_get_string_value(config, "GRADO_MULTIPROGRAMACION_INI"));

	if(nuevo_grado_multiprogramacion == grado_multiprogramacion_inicial){
		log_warning(logger, "El grado de multiprogramacion ingresado es igual al inicial.");
	} else {
		if(nuevo_grado_multiprogramacion > grado_multiprogramacion_inicial){
			for(int i=grado_multiprogramacion_inicial; i<nuevo_grado_multiprogramacion; i++){
				sem_post(&sem_multiprogramacion);
			}
		} else {
			for(int i=nuevo_grado_multiprogramacion; i<grado_multiprogramacion_inicial; i++){
				sem_wait(&sem_multiprogramacion);
			}
		}
	}
}

void listar_estados_proceso(){
	//T_QUEUE
	t_list* copy_list_new = queue_to_list_copy(procesos_en_new);
	t_list* lista_new = obtener_lista_pid(copy_list_new);
	char* new = list_to_string(lista_new);
	list_destroy(copy_list_new);
	list_destroy(lista_new);

	t_list* copy_list_exec = queue_to_list_copy(procesos_en_exec);
	t_list* lista_exec = obtener_lista_pid(copy_list_exec);
	char* exec = list_to_string(lista_exec);
	list_destroy(copy_list_exec);
	list_destroy(lista_exec);

	//T_LIST
	t_list* lista_procesos_ready = list_create();
	list_add_all(lista_procesos_ready, procesos_en_ready);
	t_list* lista_ready = obtener_lista_pid(lista_procesos_ready);
	char* ready = list_to_string(lista_ready);
	list_destroy(lista_procesos_ready);
	list_destroy(lista_ready);

	//TODO HAY VARIAS LISTAS DE BLOCKED
	t_list* lista_procesos_blocked = list_create();
	list_add_all(lista_procesos_blocked, procesos_en_blocked);
	t_list* lista_blocked = obtener_lista_pid(lista_procesos_blocked);
	char* blocked = list_to_string(lista_blocked);
	list_destroy(lista_procesos_blocked);
	list_destroy(lista_blocked);

	t_list* lista_procesos_exit = list_create();
	list_add_all(lista_procesos_exit, procesos_en_exit);
	t_list* lista_exit = obtener_lista_pid(lista_procesos_exit);
	char* exit = list_to_string(lista_exit);
	list_destroy(lista_procesos_exit);
	list_destroy(lista_exit);

	log_info(logger, "Estado NEW: [%s]\nEstado READY: [%s]\nEstado EXEC: [%s]\nEstado BLOCKED: [%s]\nEstado EXIT: [%s]", new, ready, exec, blocked, exit);
	free(new);
	free(ready);
	free(exec);
	free(blocked);
	free(exit);
}

t_list* queue_to_list_copy(t_queue* original) {
    t_list* copy_list = list_create();

    t_link_element* current_element = original->elements->head;

    while (current_element != NULL) {
        t_pcb* pcb_copy = pcb_copy_function((t_pcb*)current_element->data);
        list_add(copy_list, pcb_copy);
        current_element = current_element->next;
    }

    return copy_list;
}

t_pcb* pcb_copy_function(t_pcb* original) {
    // Verifica si el puntero original es válido
    if (original == NULL) {
        return NULL;
    }

    // Crea un nuevo t_pcb y asigna los valores del original al nuevo
    t_pcb* copy = malloc(sizeof(t_pcb));
    if (copy != NULL) {
        copy->pid = original->pid;
        copy->program_counter = original->program_counter;
        copy->prioridad = original->prioridad;
        copy->estado = original->estado;
        copy->registros_generales_cpu = original->registros_generales_cpu;
    }

    return copy;
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

bool is_pid_equal(void *element, int target_pid) {
    t_pcb *pcb = (t_pcb *)element;
    return (pcb->pid == target_pid);
}

bool hay_procesos_bloqueados_por_recursos(){
	for(int i = 0; i<list_size(lista_recursos); i++){
		t_recurso* recurso = list_get(lista_recursos, i);
		if(!list_is_empty(recurso->cola_block_asignada)){
			return true;
		}
	}
	return false;
}

//TODO Solo busca procesos bloqueados por recursos.
t_pcb* buscar_proceso_a_finalizar(int target_pid){
	/* Busca primero en los bloqueados por recursos */
	for(int i = 0; i<list_size(lista_recursos); i++){
		t_recurso* recurso = list_get(lista_recursos, i);
		t_pcb* pcb_buscado = list_get(recurso->cola_block_asignada, 0);
		if(pcb_buscado->pid == target_pid){
			recurso->instancias++;
			return list_pop_con_mutex(recurso->cola_block_asignada, &recurso->mutex_asignado);
		}
	}
}

void finalizar_proceso(char *args[]){
	int target_pid = atoi(args[1]);
	t_pcb* pcb_a_finalizar = buscar_proceso_a_finalizar(target_pid);
	pcb_a_finalizar->motivo_exit = EXIT_CONSOLA;
	list_push_con_mutex(procesos_en_exit, pcb_a_finalizar, &mutex_lista_exit);
	sem_post(&sem_procesos_exit);
}

void liberar_recursos(t_pcb* proceso){
	t_recurso* recurso_buscado = NULL;

	for(int i = 0; i<list_size(lista_recursos); i++){
		t_recurso_asignado* recurso_asignado = list_get(proceso->recursos_asignados, i);
//		log_warning(logger, "Recurso asignado: %s. Instancias: %d.", recurso_asignado->nombre_recurso, recurso_asignado->instancias);
		if(recurso_asignado->instancias > 0){
			recurso_buscado = buscar_recurso(recurso_asignado->nombre_recurso);
			recurso_buscado->instancias ++;
		}
	}

	if(recurso_buscado != NULL){
		t_pcb* pcb2 = list_pop_con_mutex(recurso_buscado->cola_block_asignada, &recurso_buscado->mutex_asignado);
		agregar_recurso(recurso_buscado->recurso, pcb2);

		pasar_a_ready(pcb2);
		sem_post(&sem_procesos_ready);
	}
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

	pcb->motivo_exit = PROCESO_ACTIVO;

	pcb->recursos_asignados = iniciar_recursos_en_proceso();
	return pcb;
}

t_list* iniciar_recursos_en_proceso(){
	t_list* lista = list_create();
	char** recursos = config_get_array_value(config, "RECURSOS");
	int cantidad_recursos = string_array_size(recursos);
	for(int i = 0; i<cantidad_recursos; i++){
		char* string = recursos[i];
		t_recurso_asignado* recurso = malloc(sizeof(t_recurso_asignado));
		recurso->nombre_recurso = malloc(sizeof(char) * strlen(string) + 1);
		strcpy(recurso->nombre_recurso, string);
		recurso->instancias = 0;
		list_add(lista, recurso);
//		log_warning(logger, "Se inicio %s con %d instancias.", recurso->nombre_recurso, recurso->instancias);
	}

	string_array_destroy(recursos);

	return lista;
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
	pthread_t hilo_respuesta_cpu;
	pthread_t hilo_blocked;
	pthread_t hilo_exit;
	pthread_create(&hilo_ready, NULL, (void*) planificar_procesos_ready, NULL);
	pthread_create(&hilo_respuesta_cpu, NULL, (void*) procesar_respuesta_cpu, NULL);
	pthread_create(&hilo_blocked, NULL, (void*) procesar_vuelta_blocked, NULL);
	pthread_create(&hilo_exit, NULL, (void*) procesar_exit, NULL);
	pthread_detach(hilo_ready);
	pthread_detach(hilo_respuesta_cpu);
	pthread_detach(hilo_blocked);
	pthread_detach(hilo_exit);
}

void procesar_vuelta_blocked(){
	while(1){
		sem_wait(&sem_vuelta_blocked);
		t_pcb* pcb = list_pop_con_mutex(procesos_en_blocked, &mutex_lista_blocked);
		pasar_a_ready(pcb);
		sem_post(&sem_procesos_ready);
	}
}

void procesar_exit(){
	while(1){
		sem_wait(&sem_procesos_exit);
		t_pcb* pcb = list_pop_con_mutex(procesos_en_exit, &mutex_lista_exit);
		char* motivo = motivo_to_string(pcb->motivo_exit);
		log_info(logger, "Finaliza el proceso %d - Motivo: %s", pcb->pid, motivo);
		sem_post(&sem_multiprogramacion);
		liberar_recursos(pcb);

		pcb_destroy(pcb);
//		TODO Terminar proceso en memoria
	}
}

void procesar_respuesta_cpu(){
	while(PLANIFICACION_ACTIVA){
		int cod_op = recibir_operacion(fd_cpu_dispatch);

		switch(cod_op){
		case PCB:
			t_pcb* pcb_actualizado = recv_pcb(fd_cpu_dispatch);

			recv(fd_cpu_dispatch, &cod_op, sizeof(op_code), 0);

			switch(cod_op){
			case CAMBIAR_ESTADO:
				estado nuevo_estado = recv_cambiar_estado(fd_cpu_dispatch);
				procesar_cambio_estado(pcb_actualizado, nuevo_estado);
				sem_post(&sem_proceso_exec);
				break;
			case ATENDER_SLEEP:
				int retardo_bloqueo = recv_sleep(fd_cpu_dispatch);
				atender_sleep(pcb_actualizado, retardo_bloqueo);
				break;
			case ATENDER_WAIT:
				char* recurso_wait = recv_recurso(fd_cpu_dispatch);
				atender_wait(pcb_actualizado, recurso_wait);
				free(recurso_wait);
				break;
			case ATENDER_SIGNAL:
				char* recurso_signal = recv_recurso(fd_cpu_dispatch);
				atender_signal(pcb_actualizado, recurso_signal);
				free(recurso_signal);
				break;
			case FOPEN:
				t_peticion* peticion_fopen = recv_peticion(fd_cpu_dispatch);
				t_archivo_abierto_global* archivo_encontrado = buscar_archivo_abierto(peticion_fopen->nombre_archivo);
				log_info(logger, "Abrir Archivo: %s", peticion_fopen->nombre_archivo);
				if (archivo_encontrado != NULL) {
					t_lock* lock_actual = list_get(archivo_encontrado->locks, 0);
					/*MODO ACTUAL ES "WRITE"*/
					if (strcmp(lock_actual->modo_lock, "W") == 0) {
						/*MODO PETICION ES "READ"*/
						if(strcmp(peticion_fopen->modo_apertura, "R") == 0){
							for(int i = 0; i<list_size(archivo_encontrado->locks); i++){
								if(!list_is_empty(archivo_encontrado->locks)){
									t_lock* lock_actual = list_get(archivo_encontrado->locks, i);
									if(strcmp(lock_actual->modo_lock, "R") == 0){
										lock_actual->cantidad_participantes ++;
									} else {
										t_lock* lock_nuevo = malloc(sizeof(t_lock));
										lock_nuevo->modo_lock = "R";
										lock_nuevo->cantidad_participantes = 0;
										list_add(archivo_encontrado->locks, lock_nuevo);
									}
								}
							}

							cambiar_estado(pcb_actualizado, BLOCKED);
							pcb_actualizado->motivo_exit = BLOCKED_READ;
							list_push_con_mutex(archivo_encontrado->lista_bloqueados, pcb_actualizado, &archivo_encontrado->mutex_cola_bloqueados);
//							log_info(logger, "PID: %d - Bloqueado por %s", pcb_actualizado->pid, peticion_fopen->nombre_archivo);
							sem_post(&sem_proceso_exec);
						} else { /* Modo de apertura W */
							t_lock* lock_nuevo = malloc(sizeof(t_lock));
							lock_nuevo->modo_lock = "W";
							lock_nuevo->cantidad_participantes = 1;
							list_add(archivo_encontrado->locks, lock_nuevo);

							cambiar_estado(pcb_actualizado, BLOCKED);
							pcb_actualizado->motivo_exit = INVALID_WRITE;
							list_push_con_mutex(archivo_encontrado->lista_bloqueados, pcb_actualizado, &archivo_encontrado->mutex_cola_bloqueados);
//							log_info(logger, "PID: %d - Bloqueado por %s", pcb_actualizado->pid, peticion_fopen->nombre_archivo);
							sem_post(&sem_proceso_exec);
						}

					/*MODO ACTUAL ES "READ"*/
					} else if (strcmp(peticion_fopen->modo_apertura, "R") == 0) {
						/*PETICION ACTUAL "READ"*/
						// El archivo está abierto en modo lectura, se solicita modo lectura permitir el acceso
						lock_actual->cantidad_participantes ++;
						log_info(logger, "PID: %d - Abrir Archivo: %s", pcb_actualizado->pid, peticion_fopen->nombre_archivo);
					} else {
						/*PETICION ACTUAL "WRITE"*/
						t_lock* lock_nuevo = malloc(sizeof(t_lock));
						lock_nuevo->modo_lock = "W";
						lock_nuevo->cantidad_participantes = 1;
						list_add(archivo_encontrado->locks, lock_nuevo);

						cambiar_estado(pcb_actualizado, BLOCKED);
						pcb_actualizado->motivo_exit = INVALID_WRITE;
						list_push_con_mutex(archivo_encontrado->lista_bloqueados, pcb_actualizado, &archivo_encontrado->mutex_cola_bloqueados);
						log_info(logger, "PID: %d - Bloqueado por %s", pcb_actualizado->pid, peticion_fopen->nombre_archivo);
						sem_post(&sem_proceso_exec);
					}
					} else {
					// El archivo no fue encontrado
					// se envia la peticion al fs
					send_peticion(fd_filesystem, pcb_actualizado ,peticion_fopen, FOPEN);
					// agrego a la lista de espera de fs
					log_info(logger, "se envio la peticion de fopen");
					list_push_con_mutex(procesos_espera_fs, pcb_actualizado ,&mutex_espera_fs);
					log_info(logger, "se agrego a procesos en espera");

					// ESTAS DOS VAN DESPUES DE QUE SE PROCESA LA CONEXION CON EL FS
					sem_wait(&fin_fopen);
					free(archivo_encontrado);
					t_archivo_abierto_global* archivo_nuevo = malloc(sizeof(t_archivo_abierto_global));
					archivo_nuevo->nombre_archivo = peticion_fopen->nombre_archivo;
					archivo_nuevo->locks = list_create();
					t_lock* lock = malloc(sizeof(t_lock));
					lock->modo_lock = peticion_fopen->modo_apertura;
					lock->cantidad_participantes = 1;
					log_warning(logger, "Paso el copy");
					list_add(archivo_nuevo->locks, lock);
					pthread_mutex_init(&archivo_nuevo->mutex_cola_bloqueados, NULL);
					archivo_nuevo->lista_bloqueados = list_create();
					list_add(tabla_global_archivos_abiertos, archivo_nuevo);
					log_info(logger, "SE AÑADIO EL ARCHIVO ABIERTO A TABLA GLOBAL");
					//list_add(pcb_actualizado->archivos_abiertos_proceso, archivo_abierto_proceso);
				}
				break;
				case FCLOSE:
					t_peticion* peticion_fclose = recv_peticion(fd_cpu_dispatch);
					t_archivo_abierto_global* archivo_a_cerrar = buscar_archivo_abierto(peticion_fclose->nombre_archivo);
					log_info(logger, "Cerrar Archivo: %s", peticion_fclose->nombre_archivo);
					//desbloquear(&lock_escritura_lectura)
					cerrar_archivo(pcb_actualizado,archivo_a_cerrar);
					queue_push_con_mutex(procesos_en_exec, pcb_actualizado, &mutex_cola_exec);
					send_pcb(pcb_actualizado, fd_cpu_dispatch);
					// entonces cerrar archivo libera una instancia de lock si es de lectura

					break;
				case FSEEK:
					t_peticion* peticion_fseek = recv_peticion(fd_cpu_dispatch);
					t_pcb* pcb_actualizado_fseek = recv_pcb(fd_cpu_dispatch);
					char* nombre_archivo_fseek = peticion_fseek -> nombre_archivo;
					uint32_t nuevo_puntero = peticion_fseek->posicion;
					// tengo que ubicar el puntero en base a la peticion
					t_archivo_abierto_proceso* archivo;
					strcpy(archivo->nombre_archivo, peticion_fseek->nombre_archivo);
					archivo->puntero = nuevo_puntero;
					// despues tengo que mandar el contexto de ejecucion a cpu
				break;
				case FTRUNCATE:
					t_peticion* peticion_ftruncate = recv_peticion(fd_cpu_dispatch);
					t_pcb* pcb_actualizado_ftruncate = recv_pcb(fd_cpu_dispatch);
					t_archivo_abierto_global* archivo_a_truncar = buscar_archivo_abierto(peticion_ftruncate->nombre_archivo);

					// tengo que hacer el send de todo junto
					send_peticion(fd_filesystem, pcb_actualizado_ftruncate ,peticion_ftruncate, FTRUNCATE);
					list_push_con_mutex(procesos_en_blocked, pcb_actualizado_ftruncate, &mutex_lista_blocked);
				break;
				case FREAD:
					// si entra aca es porque ya pudo pasar el lock
					t_peticion* peticion_fread = recv_peticion(fd_cpu_dispatch);
					t_pcb* pcb_actualizado_fread = recv_pcb(fd_cpu_dispatch);

					send_peticion(fd_filesystem, pcb_actualizado_fread ,peticion_fread, FREAD);
					list_push_con_mutex(procesos_en_blocked, pcb_actualizado_fread, &mutex_lista_blocked);
				break;
				case FWRITE:
					t_peticion* peticion_fwrite = recv_peticion(fd_cpu_dispatch);
					t_pcb* pcb_actualizado_fwrite = recv_pcb(fd_cpu_dispatch);
					if (strcmp(peticion_fwrite->modo_apertura, "R") == 0) {
						// SI SE SOLICITO EN MODO LECTURA Y SE QUIERE ESCRIBIR, FINALIZA EL PROCESO
						pcb_actualizado_fwrite->motivo_exit = INVALID_WRITE;
						cambiar_estado(pcb_actualizado_fwrite, EXIT);
						break;
					}
					// se bloquea hasta que fs informe que finalizo la operacion
					cambiar_estado(pcb_actualizado_fwrite, BLOCKED);
					send_peticion(fd_filesystem, pcb_actualizado_fwrite ,peticion_fwrite, FWRITE);
					list_push_con_mutex(procesos_en_blocked, pcb_actualizado_fwrite, &mutex_lista_blocked);
				break;
			}
		}
	}
}

void atender_sleep(t_pcb* pcb, int retardo_bloqueo){
	pthread_t hilo_sleep;
	cambiar_estado(pcb, BLOCKED);
	list_push_con_mutex(procesos_en_blocked_sleep, pcb, &mutex_lista_blocked_sleep);
	t_datos_hilo_sleep* datos_hilo_sleep = malloc(sizeof(t_datos_hilo_sleep));
	datos_hilo_sleep->pcb = pcb;
	datos_hilo_sleep->retardo_bloqueo = retardo_bloqueo;
	pthread_create(&hilo_sleep, NULL, (void*) procesar_sleep, (void*)datos_hilo_sleep);
	pthread_detach(hilo_sleep);
}

void procesar_sleep(void* args){
	t_datos_hilo_sleep* datos = (t_datos_hilo_sleep*) args;
	log_info(logger, "PID: %d - Bloqueado por: SLEEP", datos->pcb->pid);
	sem_post(&sem_proceso_exec);
//	log_info(logger, "Tiempo en sleep: %d", datos->retardo_bloqueo);
	sleep(datos->retardo_bloqueo);
	free(datos);
	t_pcb* pcb2 = list_pop_con_mutex(procesos_en_blocked_sleep, &mutex_lista_blocked_sleep);
	list_push_con_mutex(procesos_en_blocked, pcb2, &mutex_lista_blocked);
	sem_post(&sem_vuelta_blocked);
}

void atender_wait(t_pcb* pcb, char* recurso){
	t_recurso* recurso_buscado = buscar_recurso(recurso);
	if(recurso_buscado->id == -1){
		log_error(logger, "El recurso %s no existe", recurso);


		pcb->motivo_exit = INVALID_RESOURCE;
		procesar_cambio_estado(pcb, EXIT_ESTADO);
		sem_post(&sem_proceso_exec);
	} else {
		recurso_buscado->instancias --;
		log_info(logger, "PID: %d - Wait: %s - Instancias: %d", pcb->pid, recurso, recurso_buscado->instancias);
		if(recurso_buscado->instancias < 0){
			cambiar_estado(pcb, BLOCKED);
			list_push_con_mutex(recurso_buscado->cola_block_asignada, pcb, &recurso_buscado->mutex_asignado);
			sem_post(&sem_proceso_exec);
		} else {
			agregar_recurso(recurso_buscado->recurso, pcb);
			queue_push_con_mutex(procesos_en_exec, pcb, &mutex_cola_exec);
			send_pcb(pcb, fd_cpu_dispatch);
		}
	}
}

void atender_signal(t_pcb* pcb, char* recurso){
	t_recurso* recurso_buscado = buscar_recurso(recurso);
	if(recurso_buscado->id == -1){
		log_error(logger, "El recurso %s no existe", recurso);
		pcb->motivo_exit = INVALID_RESOURCE;
		procesar_cambio_estado(pcb, EXIT_ESTADO);
		sem_post(&sem_proceso_exec);
	} else {
		recurso_buscado->instancias ++;
		quitar_recurso(recurso_buscado->recurso, pcb);
		log_info(logger, "PID: %d - Signal: %s - Instancias: %d", pcb->pid, recurso, recurso_buscado->instancias);
		if(recurso_buscado->instancias <= 0){
			t_pcb* pcb2 = list_pop_con_mutex(recurso_buscado->cola_block_asignada, &recurso_buscado->mutex_asignado);
			agregar_recurso(recurso_buscado->recurso, pcb2);
			list_push_con_mutex(procesos_en_blocked, pcb2, &mutex_lista_blocked);
			sem_post(&sem_vuelta_blocked);
		}
		queue_push_con_mutex(procesos_en_exec, pcb, &mutex_lista_exit);
		send_pcb(pcb, fd_cpu_dispatch);
	}
}

void agregar_recurso(char* recurso, t_pcb* pcb){
	char** recursos = config_get_array_value(config, "RECURSOS");
	for(int i = 0; i<list_size(pcb->recursos_asignados); i++){
		t_recurso_asignado* recurso_asignado = list_get(pcb->recursos_asignados, i);
//		log_warning(logger, "Recurso asignado: %s. Instancias: %d.", recurso_asignado->nombre_recurso, recurso_asignado->instancias);
		if(strcmp(recurso_asignado->nombre_recurso, recurso) == 0){
			pthread_mutex_lock(&mutex_asignacion_recursos);
			recurso_asignado->instancias ++;
			pthread_mutex_unlock(&mutex_asignacion_recursos);
//			log_warning(logger, "Se asigno el recurso: %s. Al PID: %d. Ahora tiene %d instancias.", recurso_asignado->nombre_recurso, pcb->pid, recurso_asignado->instancias);
		}
	}

	string_array_destroy(recursos);
}

void quitar_recurso(char* recurso, t_pcb* pcb){
	char** recursos = config_get_array_value(config, "RECURSOS");
	for(int i = 0; i<list_size(pcb->recursos_asignados); i++){
		t_recurso_asignado* recurso_asignado = list_get(pcb->recursos_asignados, i);
		if(strcmp(recurso_asignado->nombre_recurso, recurso) == 0){
			pthread_mutex_lock(&mutex_asignacion_recursos);
			recurso_asignado->instancias --;
			pthread_mutex_unlock(&mutex_asignacion_recursos);
//			log_warning(logger, "Se quito el recurso: %s. Al PID: %d. Ahora tiene %d instancias.", recurso_asignado->nombre_recurso, pcb->pid, recurso_asignado->instancias);
		}
	}

	string_array_destroy(recursos);
}

t_recurso* buscar_recurso(char* recurso) {
    for (int i = 0; i < list_size(lista_recursos); i++) {
        t_recurso* recurso_buscado = list_get(lista_recursos, i);
        if (strcmp(recurso_buscado->recurso, recurso) == 0) {
            // Si se encuentra el recurso, no es necesario asignar memoria aquí.
            return recurso_buscado;
        }
    }

    t_recurso* recurso_no_encontrado = malloc(sizeof(t_recurso));
    recurso_no_encontrado->id = -1;
    return recurso_no_encontrado;
}

void procesar_cambio_estado(t_pcb* pcb, estado nuevo_estado){
	switch(nuevo_estado){
	case EXIT_ESTADO:
		cambiar_estado(pcb, nuevo_estado);
		if(pcb->motivo_exit == PROCESO_ACTIVO){
			pcb->motivo_exit = SUCCESS;
		}
		list_push_con_mutex(procesos_en_exit, pcb, &mutex_lista_exit);
		sem_post(&sem_procesos_exit);
	break;
	}
}
///////////////////////////////////// MANEJO DE LOCKS /////////////////////////////////


t_archivo_abierto_global* buscar_archivo_abierto(char* nombre_archivo){
	// tengo que buscar el archivo en la tabla global de archivos abiertos
	// tengo que ver como hago para que pase de ser un t_list a un t_archivo_abierto_global*
//	t_archivo_abierto_global* archivo_actual_tabla = malloc(sizeof(t_archivo_abierto_global));
	for (int i = 0; i < list_size(tabla_global_archivos_abiertos); i++){
		if(!list_is_empty(tabla_global_archivos_abiertos)) {
			t_archivo_abierto_global* archivo_actual_tabla = list_get(tabla_global_archivos_abiertos, i);
			log_warning(logger, "nombre archivo actual tabla: %s.", archivo_actual_tabla->nombre_archivo);
			log_warning(logger, "nombre archivo: %s.", nombre_archivo);
				if (strcmp(archivo_actual_tabla->nombre_archivo, nombre_archivo) == 0) {
					return archivo_actual_tabla;
				}
		}

	}
	return NULL;
}

void cerrar_archivo(t_pcb* pcb , t_archivo_abierto_global* archivo_a_cerrar){
	t_archivo_abierto_global* archivo_abierto;
	t_lock* lock_actual = list_get(archivo_a_cerrar->locks, 0);
	lock_actual->cantidad_participantes--;
//	if(strcmp(lock_actual->modo_lock, "R")) {
//		// si el modo de apertura es de lectura se resta un participante al lock
//		lock_actual->cantidad_participantes--;
//	} else {
//		// archivo en modo escritura
////		lock_lectura_escritura = 0;
//		lock_actual->cantidad_participantes = 0;
//	}
	if(lock_actual->cantidad_participantes == 0) {
		// se quedo sin participantes, se debe liberar el lock y cerrar el global

		for(int i=0; i<list_size(tabla_global_archivos_abiertos); i++){
			t_archivo_abierto_global* archivo_actual_tabla = list_get(tabla_global_archivos_abiertos, i);
			if (strcmp(archivo_actual_tabla->nombre_archivo, archivo_a_cerrar->nombre_archivo) == 0) {
				archivo_abierto = list_get(tabla_global_archivos_abiertos, i);
			}
		}

		if(!list_is_empty(archivo_a_cerrar->lista_bloqueados)){

			if(strcmp(lock_actual->modo_lock, "W") == 0){
				if(list_size(archivo_a_cerrar->locks) > 1){
					t_lock* lock_siguiente = list_get(archivo_a_cerrar->locks, 1);
					if(strcmp(lock_siguiente->modo_lock, "W") == 0){
						t_pcb* pcb_vuelta_blocked = list_pop_con_mutex(archivo_a_cerrar->lista_bloqueados, &archivo_a_cerrar->mutex_cola_bloqueados);
						list_push_con_mutex(procesos_en_blocked, pcb_vuelta_blocked, &mutex_lista_blocked);
						sem_post(&sem_vuelta_blocked);
						t_lock* lock = list_remove(archivo_a_cerrar->locks, 0);
						free(lock);
					} else { /*lock_siguiente en modo "R"*/
						pthread_t hilo_archivos;
						pthread_create(&hilo_archivos, NULL, (void*) procesar_vuelta_blocked_archivos, (void*)archivo_a_cerrar);
						pthread_detach(hilo_archivos);
					}
				}
			} else {
				t_pcb* pcb_vuelta_blocked = list_pop_con_mutex(archivo_a_cerrar->lista_bloqueados, &archivo_a_cerrar->mutex_cola_bloqueados);
				pcb_vuelta_blocked->motivo_exit = PROCESO_ACTIVO;
				list_push_con_mutex(procesos_en_blocked, pcb_vuelta_blocked, &mutex_lista_blocked);
				sem_post(&sem_vuelta_blocked);
				t_lock* lock = list_remove(archivo_a_cerrar->locks, 0);
				free(lock);
			}
		}
	}


	// hago lo mismo con la tabla por proceso, esto sucede si o si y no necesito que se terminen todos los participantes para cerrarlo de la tabla
//	int flag_archivo_por_proceso = 0;
//	int j = 0;
//	while (flag_archivo_por_proceso == 0) {
//		archivo_actual_proceso = list_get(pcb->archivos_abiertos_proceso, j);
//		if (strcmp(archivo_actual_proceso->nombre_archivo, archivo_a_cerrar->nombre_archivo) == 0) {
//			list_remove(pcb->archivos_abiertos_proceso, j);
//			log_info(logger, "Archivo: %s cerrado en tabla de proceso", archivo_a_cerrar->nombre_archivo);
//			flag_archivo_por_proceso = 1;
//		}
//		j++;
//	}
}

/*Trae todos los archivos bloqueados en "READ"*/
void procesar_vuelta_blocked_archivos(void* args){
	t_archivo_abierto_global* archivo_a_cerrar = (t_archivo_abierto_global*) args;
	t_list* pids_a_desbloquear = list_create();
	int lista_size = list_size(archivo_a_cerrar->lista_bloqueados);
	for(int i=0; i<lista_size; i++){
		/*SOLO FUNCIONA PARA LAS PRUEBAS, SINO HAY QUE CAMBIAR LA LOGICA Y MODIFICAR EL 0 DEL LIST_GET DE ABAJO*/
		t_pcb* pcb = list_get(archivo_a_cerrar->lista_bloqueados, 0);
		char* motivo = motivo_to_string(pcb->motivo_exit);
		if(strcmp(motivo, "BLOCKED_READ") == 0){
			t_pcb* pcb_vuelta_blocked = list_pop_con_mutex(archivo_a_cerrar->lista_bloqueados, &archivo_a_cerrar->mutex_cola_bloqueados);
			pcb_vuelta_blocked->motivo_exit = PROCESO_ACTIVO;
			list_push_con_mutex(procesos_en_blocked, pcb_vuelta_blocked, &mutex_lista_blocked);
			sem_post(&sem_vuelta_blocked);
//			list_add(pids_a_desbloquear, pcb);
		}
	}

	t_lock* lock = list_remove(archivo_a_cerrar->locks, 0);
	free(lock);
}

// Función para inicializar un lock
void inicializar_lock(t_lock* lock, char* modo_apertura) {
    strcpy(lock->modo_lock, modo_apertura);
	lock->cantidad_participantes = 0;
	list_push_con_mutex(locks_encolados, lock, &mutex_lectura_escritura);
}


bool buscar_por_nombre(void* elemento, void* nombre_buscado) {
    t_archivo_abierto_global* archivo = (t_archivo_abierto_global*)elemento;
    return strcmp(archivo->nombre_archivo, (char*)nombre_buscado) == 0;
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
//		sem_post(&sem_procesos_exit);
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

t_pcb* buscar_proceso_en_lista(t_list* lista, int pid){
	for(int i = 0; i<list_size(lista); i++){
		t_pcb* pcb_buscado = list_get(lista, i);
		if(pcb_buscado->pid == pid){
			return pcb_buscado;
		}
	}
	return NULL;
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
    t_pcb* ret = NULL;

    for (int i = 0; i < queue_size(queue); i++) {
        t_pcb* pcb_encontrado = queue_pop(queue);

        if (pcb_encontrado->pid == pid) {
            ret = pcb_encontrado;
        }
        queue_push(queue, pcb_encontrado);
    }

    return ret;
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

void generar_conexion_fs() {
	pthread_t conexion_filesystem;

	pthread_create(&conexion_filesystem, NULL, (void*) procesar_conexion_fs, (void*) &fd_filesystem);
	pthread_detach(conexion_filesystem);
}

// procesar conexion de fs es cuando ya se envia todo para que pase y el filesystem devuelve una respuesta (segun la operacion realizada)
void procesar_conexion_fs(void* void_args) {
	int* args = (int*) void_args;
	int cliente_socket = *args;

	op_code cop;
	while (cliente_socket != -1) {
		cop = recibir_operacion(cliente_socket);
		if (cop == -1) {
			log_info(logger, "El cliente se desconecto del server");
			return;
		}
		t_pcb* pcb_espera_fs = list_pop_con_mutex(procesos_espera_fs, &mutex_espera_fs);

		switch(cop){
		case FIN_FOPEN:
			int numero_recibido = recv_finalizo_fopen(cliente_socket);
			log_warning(logger, "Numero recibido para finalizacion de FOPEN: %d.", numero_recibido);
			if(numero_recibido == 1) {
				log_info(logger, "el fs termino de abrir un archivo del proceso %d", pcb_espera_fs->pid);
				sem_post(&fin_fopen);
			} else {
				log_warning(logger, "fallo");
			}

			queue_push_con_mutex(procesos_en_exec, pcb_espera_fs, &mutex_cola_exec);
			send_pcb(pcb_espera_fs, fd_cpu_dispatch);
			break;
		case FIN_FTRUNCATE:
				recv_finalizo_ftruncate(cliente_socket);
				log_info(logger, "el fs termino de truncar un archivo del proceso %d", pcb_espera_fs->pid);
				break;
		case FIN_FREAD:
			recv_finalizo_fread(cliente_socket);
			log_info(logger, "el fs termino de leer un archivo del proceso %d", pcb_espera_fs->pid);

			break;
		case FIN_FWRITE:
			recv_finalizo_fwrite(cliente_socket);

			log_info(logger, "el fs termino de escribir un archivo del proceso %d", pcb_espera_fs->pid);
			break;

		default:
			log_error(logger, "Codigo de operacion no reconocido");
			log_info(logger, "el numero del cop es: %d", cop);
			return;
		}
	}
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
	procesos_en_blocked_sleep = list_create();
	procesos_en_exit = list_create();
	lista_recursos = inicializar_recursos();

	pthread_mutex_init(&mutex_cola_new, NULL);
	pthread_mutex_init(&mutex_ready_list, NULL);
	pthread_mutex_init(&mutex_cola_exec, NULL);
	pthread_mutex_init(&mutex_planificacion_activa, NULL);
	pthread_mutex_init(&mutex_lista_blocked, NULL);
	pthread_mutex_init(&mutex_lista_blocked_sleep, NULL);
	pthread_mutex_init(&mutex_lista_exit, NULL);
	pthread_mutex_init(&mutex_asignacion_recursos, NULL);
	pthread_mutex_init(&mutex_lectura_escritura, NULL);
	pthread_mutex_init(&mutex_espera_fs, NULL);


	inicializar_semaforos();
}

void inicializar_semaforos() {
	int grado_multiprogramacion = atoi(config_get_string_value(config, "GRADO_MULTIPROGRAMACION_INI"));
	sem_init(&sem_multiprogramacion, 0, grado_multiprogramacion);
	sem_init(&sem_procesos_new, 0, 0);
	sem_init(&sem_procesos_ready, 0, 0);
	sem_init(&sem_proceso_exec, 0, 1);
	sem_init(&sem_procesos_exit, 0, 0);
	sem_init(&sem_vuelta_blocked, 0, 0);
	sem_init(&sem_procesos_blocked, 0, 0);
	sem_init(&sem_procesos_blocked_sleep, 0, 0);
	sem_init(&sem_asignacion_recursos, 0, 0);
	sem_init(&sem_vuelta_asignacion_recursos, 0, 0);
	sem_init(&fin_fopen, 0, 0);
}

t_list* inicializar_recursos(){
	t_list* lista = list_create();
	char** recursos = config_get_array_value(config, "RECURSOS");
	char** instancias = config_get_array_value(config, "INSTANCIAS_RECURSOS");
	int* instancias_recursos = string_to_int_array(instancias);
	string_array_destroy(instancias);
	int cantidad_recursos = string_array_size(recursos);
	for(int i = 0; i < cantidad_recursos; i++){
		char* string = recursos[i];
		t_recurso* recurso = malloc(sizeof(t_recurso));
		recurso->recurso = malloc(sizeof(char) * strlen(string) + 1);
		strcpy(recurso->recurso, string);
		t_list* cola_block = list_create();
		recurso->id = i;
		recurso->instancias = instancias_recursos[i];
		recurso->cola_block_asignada = cola_block;
		pthread_mutex_init(&recurso->mutex_asignado, NULL);
		list_add(lista, recurso);
	}

	free(instancias_recursos);
	string_array_destroy(recursos);
	return lista;
}

int* string_to_int_array(char** array_de_strings){
	int count = string_array_size(array_de_strings);
	int *numbers = malloc(sizeof(int) * count);
	for(int i = 0; i < count; i++){
		int num = atoi(array_de_strings[i]);
		numbers[i] = num;
	}

	return numbers;
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

void recurso_destroy(t_recurso* recurso) {
    free(recurso->recurso);
    list_destroy_and_destroy_elements(recurso->cola_block_asignada, free);
    pthread_mutex_destroy(&recurso->mutex_asignado);
    free(recurso);
}
