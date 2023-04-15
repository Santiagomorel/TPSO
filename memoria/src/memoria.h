#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <utils/utils_server.h>
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
#endif /*MEMORIA_H_*/


