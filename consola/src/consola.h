#ifndef CONSOLA_H_
#define CONSOLA_H_

#include<utils/utils.h>

char* readFile(char*,FILE*);
void leer_consola(void);
void paquete(int , char * );
void enviar_pseudocodigo(int ,int ,char* );
void terminar_programa(int, t_log*, t_config*,FILE*,char * buffer);

t_log* consola_logger;

typedef struct {
    char* ip_kernel;
    char* puerto_kernel;
} Consola_config;

Consola_config consola_config;

t_config * consola_config_file;


#endif /* CONSOLA_H_ */