#ifndef COMUNICACION_H
#define COMUNICACION_H

#include <utils/utils.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <stdbool.h>
#include <commons/config.h>

int crear_segmento_v2(int , int*);

typedef enum {
    FIRST,
    BEST,
    WORST
} t_algo_asig;

extern char* t_algo_asig_desc[3];

typedef struct {
    int base;
    int limite;
} t_esp; // Para marcar un hueco de la memoria



t_log *logger_memoria;
t_log *logger_memoria_extra;
t_config* config_memoria;

int PUERTO_ESCUCHA_MEMORIA;
int TAM_MEMORIA;
int TAM_SEGMENTO_0;
int CANT_SEGMENTOS;
int RETARDO_MEMORIA;
int RETARDO_COMPACTACION;
t_algo_asig ALGORITMO_ASIGNACION;

void* ESPACIO_USUARIO;
int ESPACIO_LIBRE_TOTAL;
t_list* LISTA_ESPACIOS_LIBRES;
t_list* LISTA_GLOBAL_SEGMENTOS;
pthread_mutex_t mutex_memoria;

sem_t finModulo;
int socket_cliente_memoria_KERNEL;
int socket_cliente_memoria_FILESYSTEM;
int socket_cliente_memoria_CPU;

void levantar_loggers_memoria();
void levantar_config_memoria();
void levantar_estructuras_administrativas();
bool comparador_base(void* data1, void* data2);
bool comparador_base_segmento(void* data1, void* data2);
void print_lista_esp(t_list* lista);
void* crear_tabla_segmentos();
char* leer(int dir_fisca , int size);
void escribir(uint32_t dir_fisca, void* data, uint32_t size);

void borrar_segmento(int base, int limite);
void* crear_tabla_segmentos();
void print_lista_segmentos();
void compactar();

void recibir_kernel(int);
void recibir_cpu(int);
void recibir_fileSystem(int);

#endif