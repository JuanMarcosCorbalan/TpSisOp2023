#include "../include/utils.h"

/**
* @NAME: queue_filter
* @DESC: Filtra la cola según la condición dada y devuelve true si encuentra al menos un elemento que cumple con la condición.
* SOLO FUNCIONA CON PID.
*/
bool queue_filter(t_queue *queue, bool (*condition)(void *, int), int target_pid) {
    if (queue == NULL || queue_is_empty(queue) || condition == NULL) {
        return false;
    }

    t_link_element *current_element = queue->elements->head;

    while (current_element != NULL) {
        t_pcb *pcb = (t_pcb *)current_element->data;
        if (condition(pcb, target_pid)) {
            // Se encontró un elemento que cumple con la condición
            return true;
        }
        current_element = current_element->next;
    }
    // No se encontró ningún elemento que cumpla con la condición
    return false;
}

//DEVUELVE EL PCB ENCONTRADO.
t_pcb *queue_find_and_remove(t_queue *queue, int target_pid) {
    if (queue == NULL || queue_is_empty(queue)) {
        return NULL;
    }

    t_link_element *current_element = queue->elements->head;
    t_link_element *prev_element = NULL;

    while (current_element != NULL) {
        t_pcb *pcb = (t_pcb *)current_element->data;

        if (pcb->pid == target_pid) {
            // Se encontró el elemento, quítalo de la cola
            if (prev_element == NULL) {
                // El elemento a eliminar es el primero
                queue->elements->head = current_element->next;
            } else {
                prev_element->next = current_element->next;
            }

            // Actualiza el tamaño de la cola si es necesario
            if (queue->elements->head == NULL) {
                // La cola está vacía, actualiza el tamaño a 0
                queue->elements->elements_count = 0;
            } else {
                // Decrementa el tamaño de la cola
                queue->elements->elements_count--;
            }

            // Guarda el puntero al PCB y libera el elemento de la cola
            t_pcb *removed_pcb = pcb;
            free(current_element);

            return removed_pcb;
        }

        prev_element = current_element;
        current_element = current_element->next;
    }

    // No se encontró el elemento
    return NULL;
}

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
		case EXIT_CONSOLA:
			return "EXIT_CONSOLA";
			break;
		default:
			return "DESCONOCIDO";
	}
}

char* estado_to_string(estado estado){
	switch(estado){
		case NEW:
			return "NEW";
			break;
		case READY:
			return "READY";
			break;
		case EXEC:
			return "EXEC";
			break;
		case BLOCKED:
			return "BLOCKED";
			break;
		case EXIT_CONSOLA:
			return "EXIT_CONSOLA";
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
