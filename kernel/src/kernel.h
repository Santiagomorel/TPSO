#ifndef KERNEL_H_
#define KERNEL_H_

#include <utils/utils.h>

#define IP_KERNEL "127.0.0.1"
#define PUERTO_KERNEL ""

// void start_kernel(void);
void load_config(void);
// void end_program(int, t_log*, t_config*);
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

int socket_cliente;
int socket_servidor_kernel;
int memory_connection;
int cpu_dispatch_connection;
int file_system_connection;

void recibir_consola(int);
t_pcb* iniciar_pcb(int );
t_pcb* pcb_create(char* , int , int );
void generar_id(t_pcb* );
char** separar_inst_en_lineas(char* );
char** parsearPorSaltosDeLinea(char* );

int contador_id = 60;

// Semaforos
pthread_mutex_t m_contador_id;
pthread_mutex_t m_listaNuevos;
// void iterator(char*);

void inicializarListasGlobales(void );

// Listas de estados de tipo de planificacion
t_list* listaNuevos;        // NEW
t_list* listaReady;         // READY
t_list* listaBloqueados;    // BLOCKED
t_list* listaEjecutando;    // RUNNING (EXEC)
t_list* listaFinalizados;   // EXIT   
t_list* listaIO;
#endif /* KERNEL_H_ */