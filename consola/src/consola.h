#ifndef CONSOLA_H_
#define CONSOLA_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<utils/utils_client.h>
#include<utils/utils_start.h>
#include <valgrind/valgrind.h>
#include <sys/types.h>
#include <sys/stat.h>

char* readFile(char*,FILE*);
void leer_consola(void);
void paquete(int,char*);
void terminar_programa(int, t_log*, t_config*,FILE*,char * buffer);

t_log* consola_logger;

typedef struct {
    char* ip_kernel;
    char* puerto_kernel;
} Consola_config;

Consola_config consola_config;

t_config * consola_config_file;


#endif /* CONSOLA_H_ */
