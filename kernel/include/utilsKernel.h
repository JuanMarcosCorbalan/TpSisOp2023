#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/queue.h>
#include <semaphore.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

//ENUMS

typedef enum{

	// los que usa el CPU
	PCB_A_EJECUTAR,

	F_OPEN,
	F_CLOSE,
	F_SEEK,
	F_READ,
	F_WRITE,
	F_TRUNCATE,
	YIELD,
	EXIT,

} t_msj_kernel_cpu;



#endif
