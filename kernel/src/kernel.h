#ifndef KERNEL_H_
#define KERNEL_H_

#include <utils/utils_client.h>
#include <utils/utils_server.h>
#include <utils/utils_config.h>

#define IP_KERNEL "127.0.0.1"
#define PUERTO_KERNEL ""

void load_config(void);
void end_program(int, t_log*, t_config*);
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

int socket_kernel;

t_log * kernel_logger;
t_config * kernel_config_file;

#endif /* KERNEL_H_ */