#ifndef KERNEL_H_
#define KERNEL_H_

#include <utils/utils_client.h>
#include <utils/utils_server.h>
#include <utils/utils_config.h>

typedef struct{

    char* ip_memoria;
    char* puerto_memoria;
    char* ip_file_system;
    char* puerto_file_system;
    char* ip_cpu;
    char* puerto_cpu;
    char* puerto_escucha;
    char* algoritmo_planificacion;

    int estimacion_inicial;

    float hrrn_alfa;

    int grado_max_multiprogramacion;

    char* recursos;
    char* instancias_recursos;

    char* ip_kernel;
    char* puerto_kernel;
} Kernel_config;

Kernel_config kernel_config;

t_log * kernel_logger;
t_config * kernel_config_file;

void load_config();
#endif /* KERNEL_H_ */