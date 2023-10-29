#include "../include/utils.h"

void imprimirPrueba(){
	printf("Prueba hello world");
}

char *list_to_string(t_list *list){
    char *string = string_new();
    for (int i = 0; i < list_size(list); i++)
    {
        int *num = (int *)list_get(list, i);
        if (i < list_size(list) - 1)
        {
            string_append_with_format(&string, "%d,", *num);
        }
        else
        {
            string_append_with_format(&string, "%d", *num);
        }
    }
    return string;
}

char* motivo_to_string(estado estado_exit){
	switch(estado_exit){
		case SUCCESS:
			return "SUCCESS";
			break;
		case INVALID_RESOURCE:
			return "INVALID_RESOURCE";
			break;
		case INVALID_WRITE:
			return "INVALID_WRITE";
			break;
		default:
			return "DESCONOCIDO";
	}
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
