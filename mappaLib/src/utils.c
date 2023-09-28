#include "utils.h"

void imprimirPrueba(){
	printf("Prueba hello world");
}

t_pcb* recibir_pcb(int socket){

	size_t size_payload;

	if(recv(socket,&size_payload,sizeof(size_t),0) != sizeof(size_t)){
		exit(EXIT_FAILURE);
	}

	void* stream_PCB_a_recibir = malloc(size_payload);

	if(recv(socket,stream_PCB_a_recibir,size_payload,0) != size_payload){
		free(stream_PCB_a_recibir);
		exit(EXIT_FAILURE);
	}

	t_pcb* pcb = deserializarPCB(stream_PCB_a_recibir);

	free();
	return pcb;

}

t_pcb* deserializarPCB(void* stream){
	 t_pcb* pcb = (t_pcb*)malloc(sizeof(t_pcb));

	 int desplazamiento = 0;

	 memcpy(&(pcb->pid), stream + desplazamiento, sizeof(pcb->pid));
	 desplazamiento += sizeof(pcb->pid);

	 memcpy(&(pcb->program_counter), stream + desplazamiento, sizeof(pcb->program_counter));
	 desplazamiento += sizeof(pcb->program_counter);

	 memcpy(&(pcb->prioridad), stream + desplazamiento, sizeof(pcb->prioridad));
	 desplazamiento += sizeof(pcb->prioridad);

	 pcb->registros_generales_cpu = deserializar_registros_generales_cpu(stream + desplazamiento);
	 desplazamiento += sizeof(t_registros_generales_cpu);

	 pcb->archivos_abiertos = deserializar_archivos_abiertos(stream + desplazamiento);
	 desplazamiento += sizeof(t_archivos_abiertos);

	 memcpy(&(pcb->estado), stream + desplazamiento, sizeof(pcb->estado));
	 desplazamiento += sizeof(pcb->estado);

	 memcpy(&(pcb->tiempo_inicial_ejecucion), stream + desplazamiento, sizeof(pcb->tiempo_inicial_ejecucion));

	 return pcb;
}

t_registros_generales_cpu deserializar_registros_generales_cpu(void* stream) {
    t_registros_generales_cpu registros;
    int desplazamiento = 0;

    memcpy(&(registros.ax), stream + desplazamiento, sizeof(registros.ax));
    desplazamiento += sizeof(registros.ax);
    memcpy(&(registros.bx), stream + desplazamiento, sizeof(registros.bx));
    desplazamiento += sizeof(registros.bx);
    memcpy(&(registros.cx), stream + desplazamiento, sizeof(registros.cx));
    desplazamiento += sizeof(registros.cx);
    memcpy(&(registros.dx), stream + desplazamiento, sizeof(registros.dx));

    return registros;
}

t_archivos_abiertos deserializar_archivos_abiertos(void* stream) {
    t_archivos_abiertos archivos;
    int desplazamiento = 0;

    memcpy(&(archivos.fd), stream + desplazamiento, sizeof(archivos.fd));
    desplazamiento += sizeof(archivos.fd);
    memcpy(&(archivos.f_pointer), stream + desplazamiento, sizeof(archivos.f_pointer));

    return archivos;
}


// PCB

void enviar_pcb(int socket, t_pcb* pcb, t_msj_kernel_cpu op_code, char** parametros_de_instruccion){
	size_t size_total;

	void* stream_pcb_a_enviar = serializar_pcb(pcb, &size_total, op_code, parametros_de_instruccion); //TODOOOOOOO

	send(socket, stream_pcb_a_enviar, size_total, 0);

	free(stream_pcb_a_enviar);
}

// LIBERAR DATOS

void liberar_pcb(t_pcb* pcb){
	// TODO HAY QUE VER SI MODELAMOS BIEN LAS COSAS DEL PCB Y A PARTIR DE AHI LO PODEMOS LIBERAR
}

void destruir_instruccion(t_instruccion* instruccion) {

}

void destruir_archivo_abierto(){

}

void liberar_parametros(char** parametros) {
	for(int i = 0; i < string_array_size(parametros); i++) {
		free(parametros[i]);
	}

	free(parametros);
}

// LISTAS DE ESTADOS

void* list_pop_con_mutex(t_list* lista, pthread_mutex_t* mutex){
	  pthread_mutex_lock(mutex);
	  void* elemento = list_remove(lista, 0);
	  pthread_mutex_unlock(mutex);
	  return elemento;
}

void list_push_con_mutex(t_list* lista, void* elemento, pthread_mutex_t* mutex) {
    pthread_mutex_lock(mutex);
    list_add(lista, elemento);
    pthread_mutex_unlock(mutex);
    return;
}

void* queue_pop_con_mutex(t_queue* queue, pthread_mutex_t* mutex) {
    pthread_mutex_lock(mutex);
    void* elemento = queue_pop(queue);
    pthread_mutex_unlock(mutex);
    return elemento;
}

void queue_push_con_mutex(t_queue* queue, void* elemento, pthread_mutex_t* mutex) {
    pthread_mutex_lock(mutex);
    queue_push(queue, elemento);
    pthread_mutex_unlock(mutex);
    return;
}
