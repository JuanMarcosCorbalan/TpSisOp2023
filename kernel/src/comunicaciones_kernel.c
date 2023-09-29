/*
 * comunicaciones_kernel.c
 *
 *  Created on: Sep 27, 2023
 *      Author: utnso
 */

#include "../include/comunicaciones_kernel.h"

t_msj_kernel_cpu esperar_cpu(int socket_cpu){
	t_msj_kernel_cpu respuesta;
	recv(socket_cpu, &respuesta, sizeof(t_msj_kernel_cpu),MSG_WAITALL);
	return respuesta;
}

//char** recibir_parametros_de_instruccion(){
//	size_t cantidadDeParametros;
//	recv(socket_cpu,&cantidadDeParametros,sizeof(size_t),MSG_WAITALL);
//	char** parametros = string_array_new();
//
//	size_t tamanioParametro;
//	char* parametroAuxiliar;
//
//	for(int i = 0; i < cantidadDeParametros; i++){
//		recv(socket_cpu,&tamanioParametro,sizeof(size_t),MSG_WAITALL);
//		parametroAuxiliar = malloc(tamanioParametro);
//		recv(socket_cpu,parametroAuxiliar, tamanioParametro,MSG_WAITALL);
//
//		string_array_push(&parametros,parametroAuxiliar);
//
//	}
//
//	return parametros;
//
//}



