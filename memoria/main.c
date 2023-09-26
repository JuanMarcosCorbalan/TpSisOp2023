#include "include/main.h"

int main() {
	logger = log_create("memoria.log", "MEMORIA", true, LOG_LEVEL_INFO);
	t_config* config;
	config = iniciar_config();
	char* puerto_escucha;
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	int server_fd = iniciar_servidor(puerto_escucha);
	log_info(logger, "MEMORIA LISTO...");
    while (experar_clientes(server_fd));

    return 0;
}

codigo_instruccion instruccion_to_enum(char* instruccion){
	if(strcmp(instruccion, "SET") == 0){
		return SET;
	} else if(strcmp(instruccion, "SUM") == 0){
		return SUM;
	} else if(strcmp(instruccion, "SUB") == 0){
		return SUB;
	}  else if(strcmp(instruccion, "EXIT") == 0){
		return EXIT;
	}

	return EXIT_FAILURE;
}

t_list* generar_instrucciones(char* path) {
	t_list* instrucciones = list_create();
	char linea[MAX_LINE_LENGTH];
	char ** token_instruccion;
	int cant_parametros;
	FILE* archivo = fopen(path, "r");

	while(fgets(linea, MAX_LINE_LENGTH, archivo)){
		t_instruccion* instruccion = malloc(sizeof(t_instruccion));
		//Eliminar el ultimo \n
		size_t longitud = strcspn(linea, "\n");
		linea[longitud] = '\0';

		token_instruccion = string_split(linea, " ");
		for(int i=0; token_instruccion[i] != NULL; i++){
			cant_parametros=i;
		}

		instruccion->codigo = instruccion_to_enum(token_instruccion[0]);
		if(cant_parametros == 0){
			instruccion->param1 = NULL;//" "??
			instruccion->param2 = NULL;
		} else if(cant_parametros == 1){
			instruccion->param1 = token_instruccion[1];
			instruccion->param2 = NULL;
		} else if(cant_parametros == 2){
			instruccion->param1 = token_instruccion[1];
			instruccion->param2 = token_instruccion[2];;
		}
		list_add(instrucciones, instruccion);
		//TODO Faltan los free??
	}
	return instrucciones;
}

t_config* iniciar_config(void)
{
	t_config* nuevo_config = config_create("./memoria.config");

	if(nuevo_config == NULL){
		printf("No se pudo leer el config\n");
		exit(2);
	}

	return nuevo_config;
}

//void liberar_proceso(int server_fd)
//{
// 	t_datos_proceso* proceso_a_finalizar;
//	recv_datos_proceso(server_fd);
//
//}
