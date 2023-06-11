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
t_pcb* pcb_create(char* , int );
void generar_id(t_pcb* );
t_registro * crear_registros(void);
char** separar_inst_en_lineas(char* );
char** parsearPorSaltosDeLinea(char* );
void enviar_Fin_consola(int);
bool bloqueado_termino_io(t_pcb *);
char * obtenerEstado(estados);
int obtenerPid(t_pcb *);
t_pcb* mayorRR (t_pcb*,t_pcb*);
t_pcb* mayorRRdeLista ( void*,void*);
double calculoEstimado (t_pcb*);
int calcularRR(t_pcb* );
void iniciar_tiempo_ejecucion(t_pcb*);
void agregar_a_lista_con_sems(t_pcb *, t_list *, pthread_mutex_t );


int contador_id = 60;

// Semaforos
sem_t proceso_en_ready;
sem_t grado_multiprog;
pthread_mutex_t m_contador_id;
pthread_mutex_t m_listaNuevos;
pthread_mutex_t m_listaBloqueados;
pthread_mutex_t m_listaEjecutando;
pthread_mutex_t m_listaReady;
pthread_mutex_t m_listaFinalizados;
pthread_t planificadorCP;
pthread_t hiloDispatch;
// void iterator(char*);

void manejar_dispatch();

void inicializarListasGlobales(void );
void iniciarSemaforos();
void destruirSemaforos();
void planificar_sig_to_ready();
void planificar_sig_to_running();
void iniciar_planificadores();

void pedir_tabla_segmentos(void ); //MODIFICAR cuando este implementado a (t_list *)
void inicializar_estructuras(t_pcb *);
void cambiar_estado_a(t_pcb *, estados , estados );

contexto_ejecucion * obtener_ce(t_pcb *);
void copiar_registros_pcb_a_ce(t_pcb*, contexto_ejecucion*);
void copiar_registros_ce_a_pcb(contexto_ejecucion*, t_pcb*);
void copiar_instrucciones_pcb_a_ce(t_pcb *, contexto_ejecucion *);
void copiar_instrucciones_ce_a_pcb(contexto_ejecucion *, t_pcb *);
void copiar_id_pcb_a_ce(t_pcb* , contexto_ejecucion* );
void copiar_id_ce_a_pcb(contexto_ejecucion* , t_pcb* );
void copiar_PC_pcb_a_ce(t_pcb* , contexto_ejecucion* );
void copiar_PC_ce_a_pcb(contexto_ejecucion* , t_pcb* );

void actualizar_pcb(t_pcb*, contexto_ejecucion*);


// Listas de estados de tipo de planificacion
t_list* listaNuevos;        // NEW
t_list* listaReady;         // READY
t_list* listaBloqueados;    // BLOCKED
t_list* listaEjecutando;    // RUNNING (EXEC)
t_list* listaFinalizados;   // EXIT   
t_list* listaIO;

t_temporal tiempo_global;
int64_t time_stamp_calculo;

void sacar_rafaga_ejecutada(t_pcb* );
void iniciar_nueva_espera_ready(t_pcb* );

void funcion_agregar_running(t_pcb* );
#endif /* KERNEL_H_ */