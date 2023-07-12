#ifndef KERNEL_H_
#define KERNEL_H_

#include <utils/utils.h>

#define IP_KERNEL "127.0.0.1"
#define PUERTO_KERNEL ""
#define MAX_RECURSOS 20
// Variables y structs globales
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

    char** recursos;
    int* instancias_recursos;

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

// Variables globales de recursos
int cantidad_instancias;

// Declaraciones de parte inicio
void load_config();
int* convertirPunteroCaracterAEntero(char** );
void inicializarListasGlobales();
void iniciar_listas_recursos(char**);
void iniciarSemaforos();
void iniciar_semaforos_recursos(char**, int*);
void iniciar_conexiones_kernel();
void iniciar_planificadores();

// Variables de semaforos
pthread_mutex_t m_contador_id;
pthread_mutex_t m_listaNuevos;
pthread_mutex_t m_listaBloqueados;
pthread_mutex_t m_listaEjecutando;
pthread_mutex_t m_listaReady;
pthread_mutex_t m_listaFinalizados;
pthread_mutex_t* m_listaRecurso[MAX_RECURSOS];
pthread_mutex_t m_IO;
sem_t proceso_en_ready;
sem_t fin_ejecucion;
sem_t grado_multiprog;
sem_t* sem_recurso[MAX_RECURSOS];

// Variables de listas
t_list* listaNuevos;        // NEW
t_list* listaReady;         // READY
t_list* listaBloqueados;    // BLOCKED
t_list* listaEjecutando;    // RUNNING (EXEC)
t_list* listaFinalizados;   // EXIT   
t_list* listaIO;
t_list** lista_recurso; // lista que tiene listas de recursos

// Variables de hilo de planificadores
pthread_t planificadorCP;
pthread_t hiloDispatch;
pthread_t hiloMemoria;
pthread_t hiloIO;

// Declaraciones de parte consola
void recibir_consola(int);
t_pcb* iniciar_pcb(int);
t_pcb* pcb_create(char*, int);
void generar_id(t_pcb*);
char** separar_inst_en_lineas(char*);
char** parsearPorSaltosDeLinea(char*);
t_registro* crear_registros();

// Variables de parte consola
int contador_id = 60;

// Declaraciones de parte planificadores
void cambiar_estado_a(t_pcb*, estados, estados);
int obtenerPid(t_pcb*);
char* obtenerEstado(estados);
int estadoActual(t_pcb*);
void agregar_a_lista_con_sems(t_pcb*, t_list*, pthread_mutex_t);
void agregar_lista_ready_con_log(t_list*, t_pcb*, char*);


// Declaraciones de planificador to - ready
void planificar_sig_to_ready();
void inicializar_estructuras(t_pcb*);
t_list* pedir_tabla_segmentos();

// Declaraciones de planificador to - running
void planificar_sig_to_running();
void funcion_agregar_running(t_pcb*);
void iniciar_tiempo_ejecucion(t_pcb*);
void setear_estimacion(t_pcb*);

// Declaraciones calculo HRRN
t_pcb* mayorRRdeLista(void*, void*);
t_pcb* mayorRR(t_pcb*, t_pcb*);
double calcularRR(t_pcb*);
double calculoEstimado(t_pcb*);
t_pcb* mayor_prioridad_PID(t_pcb*, t_pcb*);

// Declaraciones de planificador blocked

// Declaraciones CE (contexto de ejecucion)
contexto_ejecucion* obtener_ce(t_pcb*);
void copiar_id_pcb_a_ce(t_pcb*, contexto_ejecucion*);
void copiar_instrucciones_pcb_a_ce(t_pcb*, contexto_ejecucion*);
void copiar_PC_pcb_a_ce(t_pcb*, contexto_ejecucion*);
void copiar_PC_ce_a_pcb(contexto_ejecucion*, t_pcb*);
void copiar_registros_pcb_a_ce(t_pcb*, contexto_ejecucion*);
void copiar_registros_ce_a_pcb(contexto_ejecucion*, t_pcb*);
void copiar_tabla_segmentos_pcb_a_ce(t_pcb*, contexto_ejecucion*);


// Declaraciones Dispatch Manager
void manejar_dispatch();
void actualizar_pcb(t_pcb*, contexto_ejecucion*);

// Declaraciones de fin de proceso
void atender_final_proceso(int);
void liberar_recursos_pedidos(t_pcb*);
void enviar_Fin_consola(int);

// Declaraciones DESALOJO_YIELD
void atender_desalojo_yield();
void sacar_rafaga_ejecutada(t_pcb*);
void iniciar_nueva_espera_ready(t_pcb*);

// Declaraciones manejo de recursos
int recurso_no_existe(char*);
int obtener_id_recurso(char*);
int id_proceso_en_lista(t_list*);
int obtener_instancias_recurso(int);
void restar_instancia(int);
void sumar_instancia(int);
void sumar_instancia_exit(int, t_pcb*);

// Declaraciones EJECUTAR_INSTRUCCION
void atender_ejecutar_instruccion();
// Declaraciones WAIT_RECURSO
void atender_wait_recurso();
int tiene_instancia_wait(int);
void bloqueo_proceso_en_recurso(t_pcb*, int);

// Declaraciones SIGNAL_RECURSO
void atender_signal_recurso();
int tiene_que_reencolar_bloq_recurso(int);
void reencolar_bloqueo_por_recurso(int);

// Declaraciones BLOCK_IO
typedef struct{
    t_pcb* pcb;
    int bloqueo;
}thread_args;

void atender_block_io();
void rutina_io(thread_args*);

// Declaraciones CREAR_SEGMENTO
void atender_crear_segmento();
void atender_compactacion(int, int, int);

// Declaraciones BORRAR_SEGMENTO
void atender_borrar_segmento();
// Declaraciones Memory Manager
void manejar_memoria();

//
// ------------------------------------//

// void end_program(int, t_log*, t_config*);

// Declaraciones finales
void destruirSemaforos();




#endif /* KERNEL_H_ */