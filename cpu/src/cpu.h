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



void leer_consola(t_log*);
void paquete(int);
void terminar_programa(int, t_log*, t_config*);
void establecer_conexion(char* , char* , t_config*, t_log*);

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
    int program_counter; // número de la próxima instrucción a ejecutar.
    char** cpu_registers[12]; //Estructura que contendrá los valores de los registros de uso general de la CPU. mas adelante hay que pensarla   
    int size;
	int client_socket;
    char** instructions; // lista de instrucciones a ejecutar. char**
	pcb_status status;
    t_list* segment_table;
} t_pcb;

typedef struct {
	int index_page; // Indice de la tabla de paginas que esta en memoria!!
	int number; // Numero de segmento
	int size; // Tamaño de segmento
	 
} t_segment_table; 

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