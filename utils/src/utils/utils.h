#ifndef UTILS_H_
#define UTILS_H_

/*    Includes generales    */

#include <stdio.h>
#include <stdint.h>
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
#include <commons/bitarray.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
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
	STRING,
	//RECIBIR_PCB,
	// -------  CPU->kernel --------
	EJECUTAR_CE, 			//  dispatch
	// ------- enviadas por DIspatch: (CPU->kernel) --------
	SUCCESS,
	SEG_FAULT,
	INVALID_RESOURCE,
	BLOCK_IO,
	WAIT_RECURSO,
	DESALOJO_YIELD,
	SIGNAL_RECURSO,
	ABRIR_ARCHIVO,
	CERRAR_ARCHIVO,
	LEER_ARCHIVO,
	ESCRIBIR_ARCHIVO,
	ACTUALIZAR_PUNTERO,
	MODIFICAR_TAMAÑO_ARCHIVO,
	CREAR_SEGMENTO,
	BORRAR_SEGMENTO,
	DESALOJO_ARCHIVO,
	// -------KERNEL->MEMORIA --------
	ACCEDER_TP,
	ACCEDER_EU,
	INICIAR_PROCESO,
	SUSPENDER_PROCESO, // esto no va
	CREATE_PROCESS,
	DELETE_PROCESS,
	CREATE_SEGMENT,
	DELETE_SEGMENT,
	COMPACTAR,
	SIN_ESPACIO,
	ASK_COMPACTAR,
	// -------KERNEL->FILESYSTEM --------
	F_TRUNCATE,
	F_WRITE,
	F_READ,
	F_OPEN,
	F_CREATE,
	CONSULTA_ARCHIVO,
	EXISTE_ARCHIVO,
	NO_EXISTE_ARCHIVO,
	CREAR_ARCHIVO,
	ENVIAR_CONFIG, 			//siendo el cpu le pido a la mem que me pase la configuracion para traducir las direcciones
	//MMU
	PEDIDO_INDICE_DOS, // 1er acceso
	PEDIDO_MARCO,	// 2do acceso
	PEDIDO_VALOR,
	MOV_IN,
	MOV_IN_OK,
	MOV_OUT,
	MOV_OUT_OK,
	// -------MEMORIA --------
	INICIAR_ESTRUCTURAS,
	TABLA_SEGMENTOS,
	FINALIZAR_ESTRUCTURAS,
	INDICE_2, 	// 1er acceso mmu
	MARCO,		// 2do acceso mmu
	//PAGE_FAULT,
	OUT_OF_MEMORY,
	NECESITO_COMPACTAR,
	DIR_FISICA,
	VALOR_A_RECIBIR,	

	CONFIG_MEMORIA,
	FIN_CONSOLA,		
	OK,
    FAIL = -1,
	NUEVO_FCB_OK,
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
	double estimacion_rafaga;
    t_temporal* tiempo_llegada_ready;
	t_list* tabla_archivos_abiertos_por_proceso;

	t_temporal* salida_ejecucion;
	int64_t rafaga_ejecutada;

	double calculoRR;

	int socket_consola;
	estados estado_actual;

	t_list* recursos_pedidos;
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

typedef struct{
	int id;
	t_list* tabla_segmentos;
}t_proceso;



typedef struct {
	int id;
	char** instrucciones;
	int program_counter;
	t_registro* registros_cpu;
	t_list* tabla_segmentos;
} contexto_ejecucion;

typedef struct{
	contexto_ejecucion* ce;
	int entero1;
	int entero2;
} t_ce_2enteros;

typedef struct{
	contexto_ejecucion* ce;
	int entero;
} t_ce_entero;

typedef struct{
	contexto_ejecucion* ce;
	char* string;
} t_ce_string;

typedef struct{
	contexto_ejecucion* ce;
	char* string;
	int entero;
} t_ce_string_entero;
typedef struct{
	contexto_ejecucion* ce;
	char* string;
	int entero1;
	int entero2;
} t_ce_string_2enteros;

typedef struct{
	contexto_ejecucion* ce;
	char* string;
	int entero1;
	int entero2;
	int entero3;
} t_ce_string_3enteros;

typedef struct{
	int entero1;
	int entero2;
} t_2_enteros;

typedef struct{
	char* string;
	int entero1;
} t_string_entero;
typedef struct{
	char* string;
	int entero1;
	int entero2;
} t_string_2enteros;
typedef struct{
	char* string;
	int entero1;
	int entero2;
	int entero3;
} t_string_3enteros;
typedef struct{
	char* string;
	int entero1;
	int entero2;
	int entero3;
	int entero4;
} t_string_4enteros;


typedef struct{
	int entero1;
	int entero2;
	int entero3;
} t_3_enteros;

typedef struct{
	int DF;
	char* registro;
	int PID;
	int size;
} recive_mov_out;

int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(void);
t_paquete* crear_paquete_op_code(op_code codigo_op);
t_paquete* crear_super_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void agregar_entero_a_paquete(t_paquete* , int );
void agregar_string_a_paquete(t_paquete *paquete, char* palabra);
void agregar_array_string_a_paquete(t_paquete* paquete, char** arr);
void agregar_registros_a_paquete(t_paquete* , t_registro*);
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
char** leer_string_array(char* , int* );
t_registro * leer_registros(char* , int * );

void loggear_pcb(t_pcb* , t_log* );
void loggear_estado(t_log* , int );

t_list* recibir_paquete_segmento(int );
contexto_ejecucion * recibir_ce(int );
char* recibir_string(int, t_log*);
int recibir_entero(int, t_log*);

void enviar_paquete_string(int, char*, int, int);
void enviar_paquete_entero(int , int , int );

void enviar_ce(int, contexto_ejecucion *, int, t_log*);
void enviar_CodOp(int socket, int codOP);
void enviar_paquete_entero(int, int, int);

void agregar_ce_a_paquete(t_paquete *, contexto_ejecucion *, t_log*);
contexto_ejecucion * obtener_ce(t_pcb * pcb);

void imprimir_ce(contexto_ejecucion* , t_log*);
void imprimir_registros(t_registro* , t_log*);
void imprimir_tabla_segmentos(t_list* , t_log* );

void liberar_ce(contexto_ejecucion* );
//liberar registro -> 

char* obtenerCodOP(int);

t_proceso* recibir_tabla_segmentos_como_proceso(int, t_log*);
t_list* recibir_tabla_segmentos(int);
t_list* leer_tabla_segmentos(char*, int*);

t_segmento* crear_segmento(int, int, int);

void enviar_string_entero(int,char*,int,int codOP);
void enviar_string_enterov2(int,char*,int,int codOP);
void agregar_tabla_segmentos_a_paquete(t_paquete*, t_list*);

t_ce_2enteros * recibir_ce_2enteros(int);
t_ce_entero* recibir_ce_entero(int);
t_ce_string* recibir_ce_string(int);
t_ce_string_entero* recibir_ce_string_entero(int);
void enviar_2_enteros(int client_socket, int x, int y, int codOP);
t_string_entero* recibir_string_entero(int);
t_string_entero* recibir_string_enterov2(int);
t_2_enteros * recibir_2_enteros(int);
void enviar_3enteros(int client, int x, int y, int z, int codOP);
void enviar_string_2enteros(int, char*, int, int, int);
void enviar_string_3enteros(int client, char* string, int x, int y, int z, int codOP);
void enviar_string_4enteros(int client, char* string, int x, int y, int z, int j, int codOP);
t_ce_string_3enteros * recibir_ce_string_3enteros(int socket);
void enviar_3_enteros(int client_socket, int x, int y, int z, int codOP);
t_ce_string_2enteros* recibir_ce_string_2enteros(int);
t_3_enteros * recibir_3_enteros(int);
t_string_3enteros* recibir_string_3enteros(int);
t_string_4enteros* recibir_string_4enteros(int);
recive_mov_out * recibir_mov_out(int);
void liberar_ce_2enteros(t_ce_2enteros*);
void liberar_ce_entero(t_ce_entero*);
void liberar_ce_string(t_ce_string*);
void liberar_ce_string_entero(t_ce_string_entero*);
void liberar_ce_string_2enteros(t_ce_string_2enteros*);
void enviar_todas_tablas_segmentos(int, t_list*, int, t_log*);
t_list* recibir_todas_tablas_segmentos(int);
t_proceso* recibir_t_proceso(char*, int*);
#endif /* UTILS_H_ */