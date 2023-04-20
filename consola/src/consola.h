#ifndef CONSOLA_H_
#define CONSOLA_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<utils/utils_client.h>
#include <valgrind/valgrind.h>
#include <sys/types.h>
#include <sys/stat.h>

char* readFile(char*,FILE*,t_log*);
t_log* iniciar_logger(void);
t_config* iniciar_config(void);
void leer_consola(t_log*);
void paquete(int,char*);
void terminar_programa(int, t_log*, t_config*,FILE*,char * buffer);

#endif /* CONSOLA_H_ */
