#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <utils/utils.h>
//typedef enum{
//    KERNEL,
//    CPU,
//    FILESYSTEM,
//    OTRO
//}cod_mod;

//cod_mod recibir_handshake(int socket_cliente){
//    cod_mod rta_handshake;
//
//    recv(socket_cliente, &rta_handshake, sizeof(cod_mod), MSG_WAITALL);
//    return rta_handshake;
//}

typedef struct{

    char* puerto_escucha;
    char* tam_memoria;
    int tam_segmento;
    char* cant_segmentos;
    char* retardo_memoria;
    char* retardo_compactacion;
    char* algoritmo_asignacion;

} Memoria_config;
Memoria_config memoria_config;

//typedef struct{
//    int32_t PID;
//    t_list segmentos;
//}t_tabla_segmentos;
t_segmento* crear_segmento(int ,int ,int );

int socket_servidor_memoria;
int socket_cliente_memoria_CPU;
int socket_cliente_memoria_FILESYSTEM;
int socket_cliente_memoria_KERNEL;

void* MEMORIA_PRINCIPAL;

t_config* memoria_config_file;
t_log* log_memoria;

void load_config(void);

void end_program(int ,t_log* ,t_config* );

void recibir_kernel(int);
void recibir_cpu(int);
void recibir_fileSystem(int);

void enviar_tabla_segmentos();
void* serializar_segmento(t_segmento* segmento);
#endif /*MEMORIA_H_*/


