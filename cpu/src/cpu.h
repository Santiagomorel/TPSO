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
t_config * cpu_config_file;

int conexion_cpu;
int socket_cpu;


void load_config(void);
void leer_consola(t_log*);
void paquete(int);
void terminar_programa(int, t_log*, t_config*);

/*-------------- CONEXIONES ------------*/
void establecer_conexion(char* , char* , t_config*, t_log*);
void handshake_cliente(int socket_cliente);
void handshake_servidor(int socket_cliente);

/*-------------- REGISTROS ------------*/
char registers[12];

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

#define REQUEST 0
#define RESPONSE 1

void init_registers();
void set_registers(t_pcb* pcb);
void add_value_to_register(char* registerToModify, char* valueToAdd);
void add_two_registers(char* registerToModify, char* registroParaSumarleAlOtroRegistro);


/*------------------- INSTRUCCIONES --------------------*/

#define BADKEY -1
#define I_SET 1
#define I_ADD 2
#define I_IO 3
#define I_MOV_IN 4
#define I_MOV_OUT 5
#define I_EXIT 6

int keyfromstring(char *key);

/*-------------- PROCESOS -------------------*/

int check_interruption;
int page_fault;
int sigsegv;
int numeroSegmentoGlobalPageFault;
int numeroPaginaGlobalPageFault;
pthread_mutex_t m_execute_instruct;



/*-------------- CICLO DE INSTRUCCION --------------------*/

char* fetch_next_instruction_to_execute(t_pcb* pcb);
char** decode(char* linea);
void execute_instruction(char** instruccion_a_ejecutar, t_pcb* pcb);
void update_program_counter(t_pcb* pcb);
void execute_process(t_pcb* pcb);

#endif