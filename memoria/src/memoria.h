#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <utils/utils_server.h>
#include <utils/utils_config.h>
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
t_config* memoria_config_file;


#endif /*MEMORIA_H_*/


