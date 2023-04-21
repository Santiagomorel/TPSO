#ifndef CPU
#define CPU_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<utils/utils_client.h>
#include <valgrind/valgrind.h>
#include <utils/utils_server.h>
#include <utils/utils_start.h>

typedef struct{
    char* ip_memoria;
	char* puerto_memoria;
	char* retardo_instruccion;
	char* puerto_escucha;
	char* tam_max_segmento;

}CPU_config;

CPU_config cpu_config;

t_log * cpu_logger;
t_config * cpu_config_file;



void leer_consola(t_log*);
void paquete(int);
void terminar_programa(int, t_log*, t_config*);
void establecer_conexion(char* , char* , int, t_config*, t_log*);

#endif