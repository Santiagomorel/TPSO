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

void load_config(void){
    memoria_config.puerto_escucha           = config_get_string_value(memoria_config_file, "PUERTO_ESCUCHA");
    memoria_config.tam_memoria              = config_get_string_value(memoria_config_file, "TAM_MEMORIA");
    memoria_config.tam_segmento             = config_get_string_value(memoria_config_file, "TAM_SEGMENTO");
    memoria_config.cant_segmentos           = config_get_string_value(memoria_config_file, "CANT_SEGMENTOS");
    memoria_config.retardo_memoria          = config_get_string_value(memoria_config_file, "RETARDO_MEMORIA");
    memoria_config.retardo_compactacion     = config_get_string_value(memoria_config_file, "RETARDO_COMPACTACION");
    memoria_config.algoritmo_asignacion     = config_get_string_value(memoria_config_file, "ALGORITMO_ASIGNACION");
}
typedef struct{

    char* puerto_escucha;
    char* tam_memoria;
    char* tam_segmento;
    char* cant_segmentos;
    char* retardo_memoria;
    char* retardo_compactacion;
    char* algoritmo_asignacion;

} Memoria_config;
void end_program(int socket, t_log* log, t_config* config){
    log_destroy(log);
    config_destroy(config);
    liberar_conexion(socket);
}

Memoria_config memoria_config;

int socket_servidor_memoria;
int socket_cliente_memoria_CPU;
int socket_cliente_memoria_FILESYSTEM;
int socket_cliente_memoria_KERNEL;

t_config* memoria_config_file;
t_log* log_memoria;

void load_config(void);
void end_program(int, t_log*, t_config*);

void recibir_kernel(int);
void recibir_cpu(int);
void recibir_fileSystem(int);

#endif /*MEMORIA_H_*/


