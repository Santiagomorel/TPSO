#ifndef CPU
#define CPU_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<utils/utils_client.h>
#include <valgrind/valgrind.h>
#include <utils/utils_server.h>
#include <utils/utils_start.h>

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



void leer_consola(t_log*);
void paquete(int);
void terminar_programa(int, t_log*, t_config*);
void establecer_conexion(char* , char* , t_config*, t_log*);

/*-------------- REGISTROS ------------*/
char registers[4];
char registers[4];
char registers[4];

#define AX 0
#define BX 1
#define CX 2
#define DX 3
#define EAX 4
#define EBX 5
#define ECX 6
#define EDX 7
#define RAX 8
#define RBX 9
#define RCX 10
#define RDX 11

void init_registers();
void add_value_to_register(char* registerToModify, char* valueToAdd);
void add_two_registers(char* registerToModify, char* registroParaSumarleAlOtroRegistro);


/*------------------- BUFFER --------------------*/
void* receive_buffer(int*, int);


/*-------------- PCB -------------------*/

typedef enum { // Los estados que puede tener un PCB
    NEW,
    READY,
    BLOCKED,
    RUNNING,
    EXIT,
} pcb_status;
typedef struct {
    int process_id; //process id identificador del proceso.
    int size;
    int program_counter; // número de la próxima instrucción a ejecutar.
	int client_socket;
    uint32_t cpu_registers[12]; //Estructura que contendrá los valores de los registros de uso general de la CPU. mas adelante hay que pensarla
    char** instructions; // lista de instrucciones a ejecutar. char**
	pcb_status status;
} t_pcb;



t_pcb* pcb_create(char** instructions, int client_socket, int pid);
char** generate_instructions_list(char* instructions);

char* get_state_name(int state);
void logguar_state(t_log* logger, int state);

t_pcb* receive_pcb(int socket, t_log* logger);

void change_pcb_state_to(t_pcb* pcb, pcb_status newState);
void loggear_pcb(t_pcb* pcb, t_log* logger);
void loggear_registers(t_log* logger, uint32_t* registers);

int read_int(char* buffer, int* desp);

/*-------------- PROCESOS --------------------*/
char* fetch_next_instruction_to_execute(t_pcb* pcb);

#endif