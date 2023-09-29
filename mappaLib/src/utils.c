#include "../include/utils.h"

void imprimirPrueba(){
	printf("Prueba hello world");
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
