#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <utils/utils.h>
typedef enum{
    KERNEL,
    CPU,
    FILESYSTEM
}cod_mod;
typedef struct{

    char* puerto_escucha;
    char* tam_memoria;
    char* tam_segmento;
    char* cant_segmentos;
    char* retardo_memoria;
    char* retardo_compactacion;
    char* algoritmo_asignacion;

} Memoria_config;

Memoria_config memoria_config;

int socket_servidor_memoria;
int socket_cliente_memoria;

t_config* memoria_config_file;
t_log* log_memoria;

void load_config(void);
void end_program(int, t_log*, t_config*);

#endif /*MEMORIA_H_*/


