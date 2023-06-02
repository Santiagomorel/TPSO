#ifndef CPU
#define CPU_H_

#include<utils/utils.h>

typedef struct{
    char* ip_memoria;
	char* puerto_memoria;
	char* retardo_instruccion;
	char* puerto_escucha;
	char* tam_max_segmento;

}CPU_config;

CPU_config cpu_config;

t_log * cpu_logger;
t_log * mandatory_logger;
t_config * cpu_config_file;

int conexion_cpu;
int socket_cpu;
int socket_kernel;


void load_config(void);
void leer_consola(t_log*);
void paquete(int);
void terminar_programa(int, t_log*, t_config*);

/*-------------- CONEXIONES ------------*/
void establecer_conexion(char* , char* , t_config*, t_log*);
void handshake_cliente(int socket_cliente);
void handshake_servidor(int socket_cliente);

void process_dispatch();


/*-------------- REGISTROS ------------*/

/*
typedef enum{
    AX,
    BX,
    CX,
    DX,
    EAX,
    EBX,
    ECX,
    EDX,
    RAX,
    RBX,
    RCX,
    RDX
}registros;
*/


t_registro* registros;

#define REQUEST 0
#define RESPONSE 1

void init_registers();
void set_registers(contexto_ejecucion* ce);
void save_context_ce(contexto_ejecucion* ce);
void add_value_to_register(char* registerToModify, char* valueToAdd);
void add_two_registers(char* registerToModify, char* registroParaSumarleAlOtroRegistro);
void enviar_recurso(int client_socket, contexto_ejecucion* ce, char* parameter, int codOP);
void enviar_io(int client_socket, contexto_ejecucion* ce, char* parameter, int codOP);


/*------------------- INSTRUCCIONES --------------------*/

#define BADKEY -1
#define I_SET 1
#define I_IO 2
#define I_EXIT 3
#define I_WAIT 4
#define I_SIGNAL 5
#define I_YIELD 6



int keyfromstring(char *key);

/*-------------- PROCESOS -------------------*/


int check_interruption;
int page_fault;
int sigsegv;
int numeroSegmentoGlobalPageFault;
int numeroPaginaGlobalPageFault;
pthread_mutex_t m_execute_instruct;



/*-------------- CICLO DE INSTRUCCION --------------------*/

char* fetch_next_instruction_to_execute(contexto_ejecucion* ce);
char** decode(char* linea);
void execute_instruction(char** instruccion_a_ejecutar, contexto_ejecucion* ce);
void update_program_counter(contexto_ejecucion* ce);
void execute_process(contexto_ejecucion* ce);

#endif