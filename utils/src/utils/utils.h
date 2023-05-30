#ifndef UTILS_H_
#define UTILS_H_

/*    Includes generales    */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include <commons/collections/list.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <valgrind/valgrind.h>
#include <readline/readline.h>
#include <pthread.h>
#include <semaphore.h>

/*    Definiciones de Funcionalidad para Servidor    */

#define IP "127.0.0.1"

void* recibir_buffer(int*, int);

int iniciar_servidor(char*, t_log*);
int esperar_cliente(int,t_log*);
t_list* recibir_paquete(int);
void recibir_mensaje(int,t_log*);
int recibir_operacion(int);
void recieve_handshake(int);

/*    Definiciones de Funcionalidad para Cliente    */

typedef enum
{
	MENSAJE,
	PAQUETE,
	INICIAR_PCB,
	//RECIBIR_PCB,
	// -------  CPU->kernel --------
	EJECUTAR_CE, 			//  dispatch
	EJECUTAR_INTERRUPCION,	// 	interrupt
	// ------- enviadas por DIspatch: (CPU->kernel) --------
	FIN_PROCESO,
	DESALOJO_PCB,  			// TODO RUSO
	BLOCK_IO,
	// -------KERNEL->MEMORIA --------
	ACCEDER_TP,
	ACCEDER_EU,
	INICIAR_PROCESO,
	SUSPENDER_PROCESO,
	//  CPU->MEMORIA
	ENVIAR_CONFIG, 			//siendo el cpu le pido a la mem que me pase la configuracion para traducir las direcciones
	//MMU
	PEDIDO_INDICE_DOS, // 1er acceso
	PEDIDO_MARCO,	// 2do acceso
	PEDIDO_VALOR,
	WRITE,
	// -------MEMORIA --------
	INICIAR_ESTRUCTURAS,
	TABLA_SEGMENTOS,
	FINALIZAR_ESTRUCTURAS,
	INDICE_2, 	// 1er acceso mmu
	MARCO,		// 2do acceso mmu
	//PAGE_FAULT,
	DIR_FISICA,
	VALOR_A_RECIBIR,	

	CONFIG_MEMORIA,
	FIN_CONSOLA,		
	OK,
    FAIL = -1,
} op_code;

typedef enum { // Los estados que puede tener un PCB
    NEW,
    READY,
    BLOCKED,
	// BLOCKED_READY,
    RUNNING,
    EXIT,
} estados;

typedef struct{
	int id_segmento;
	int direccion_base;		//falta definir tipo
	int tamanio_segmento;
} t_segmento;

typedef struct{
	char* archivo;
	int puntero;
} t_archivo_abierto;

typedef struct{
    char AX[4];
    char BX[4];
    char CX[4];
    char DX[4];
    char EAX[8];
    char EBX[8];
    char ECX[8];
    char EDX[8];
    char RAX[16];
    char RBX[16];
    char RCX[16];
    char RDX[16];
}t_registro;

typedef struct {
    int id;
	char** instrucciones;
    int program_counter;
	t_registro* registros_cpu;
	t_list* tabla_segmentos;
	float estimacion_rafaga;
    t_temporal tiempo_llegada_ready;
	t_list* tabla_archivos_abiertos; // [t_archivo_abierto]

	t_temporal salida_ejecucion;
	t_temporal llegada_ejecucion;

	int socket_consola;
	estados estado_actual;
} t_pcb;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;


typedef struct {
	int id;
	char** instrucciones;
	int program_counter;
	t_registro* registros_cpu;
	t_list* tabla_segmentos;
} contexto_ejecucion;



int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(void);
t_paquete* crear_paquete_op_code(op_code codigo_op);
t_paquete* crear_super_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void agregar_entero_a_paquete(t_paquete* , int );
void agregar_array_string_a_paquete(t_paquete* paquete, char** arr);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void send_handshake(int socket_cliente);

/*    Definiciones de Funcionalidad para Configuracion Inicial    */

t_config* init_config(char * config_path);

t_log* init_logger(char *file, char *process_name, bool is_active_console, t_log_level level);

/*    Definiciones de Funcionalidad para Serializacion/Deserializacion    */

int leer_entero(char* , int* );
t_list* leer_segmento(char* , int* );
float leer_float(char* , int* );
char* leer_string(char* , int* );


void loggear_pcb(t_pcb* , t_log* );
void loggear_estado(t_log* , int );

t_list* recibir_paquete_segmento(int );

t_paquete* agregar_tabla_segmentos_a_paquete(t_paquete * , t_list *);

void enviar_ce(int, contexto_ejecucion *, int);

void agregar_ce_a_paquete(t_paquete *, contexto_ejecucion *);
#endif /* UTILS_H_ */