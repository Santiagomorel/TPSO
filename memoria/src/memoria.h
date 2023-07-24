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

typedef struct {
    uint32_t id_seg;
    uint32_t base;
    uint32_t tam;
	uint8_t activo;
} t_ent_ts; // Entrada de la tabla de segmentos

typedef struct{

    char* puerto_escucha;
    int tam_memoria;
    int tam_segmento_0;
    int cant_segmentos;
    int retardo_memoria;
    int retardo_compactacion;
    char* algoritmo_asignacion;

} Memoria_config;
Memoria_config memoria_config;

t_log *logger_memoria;
t_log *logger_memoria_extra;
t_config* config_memoria;



t_algo_asig ALGORITMO_ASIGNACION;


void* ESPACIO_USUARIO;
int ESPACIO_LIBRE_TOTAL;
t_list* LISTA_ESPACIOS_LIBRES;
t_list* LISTA_GLOBAL_SEGMENTOS;
pthread_mutex_t mutex_memoria;

sem_t finModulo;
int socket_servidor_memoria;
int socket_cliente_memoria_KERNEL;
int socket_cliente_memoria_FILESYSTEM;
int socket_cliente_memoria_CPU;

t_list* tabla_de_procesos;
pthread_mutex_t listaProcesos;


void levantar_loggers_memoria();
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
void enviar_tabla_segmentos(int conexion, int codOP, t_proceso* proceso);
t_proceso* crear_proceso_en_memoria(int id_proceso);
void generar_tabla_segmentos(t_proceso* proceso);
void agregar_tabla_a_paquete(t_paquete *paquete, t_proceso *proceso, t_log *logger);

void recibir_kernel(int);
void recibir_cpu(int);
void recibir_fileSystem(int);

void borrar_proceso(int PID);

#endif