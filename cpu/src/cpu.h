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
void terminar_programa(t_log*, t_config*);

/*-------------- CONEXIONES ------------*/
void establecer_conexion(char* , char* , t_config*, t_log*);
void handshake_cliente(int socket_cliente);
void handshake_servidor(int socket_cliente);

void process_dispatch();


/*-------------- REGISTROS ------------*/

t_registro* registros;

#define REQUEST 0
#define RESPONSE 1

void init_registers();
void set_registers(contexto_ejecucion* ce);
void save_context_ce(contexto_ejecucion* ce);
void add_value_to_register(char* registerToModify, char* valueToAdd);
void add_two_registers(char* registerToModify, char* registroParaSumarleAlOtroRegistro);
void enviar_ce_con_string(int client_socket, contexto_ejecucion* ce, char* parameter, int codOP);
void enviar_ce_con_string_entero(int client_socket, contexto_ejecucion* ce, char* parameter, char* x, int codOP);
void enviar_ce_con_dos_enteros(int client_socket, contexto_ejecucion* ce, char* x, char* y, int codOP);
void enviar_ce_con_entero(int client_socket, contexto_ejecucion* ce, char* parameter, int codOP);
void enviar_ce_con_string_2_enteros(int client_socket, contexto_ejecucion* ce, char* parameter, char* x, char* y, int codOP);

/*------------------- INSTRUCCIONES --------------------*/

#define BADKEY -1
#define I_SET 1
#define I_IO 2
#define I_EXIT 3
#define I_WAIT 4
#define I_SIGNAL 5
#define I_YIELD 6
#define I_F_OPEN 7
#define I_F_CLOSE 8
#define I_F_SEEK 9
#define I_F_READ 10
#define I_F_WRITE 11
#define I_F_TRUNCATE 12
#define I_CREATE_SEGMENT 13
#define I_DELETE_SEGMENT 14
#define I_MOV_IN 15
#define I_MOV_OUT 16



int keyfromstring(char *key);

/*-------------- PROCESOS -------------------*/


int check_interruption;
int sigsegv;
int sig_f;
int id_segmento_con_segfault;
int desplazamiento_segfault;
int tamanio_segfault;
pthread_mutex_t m_execute_instruct;
int recibir_respuesta_recurso();
int recibir_respuesta_segmento();


/*-------------- CICLO DE INSTRUCCION --------------------*/

char* fetch_next_instruction_to_execute(contexto_ejecucion* ce);
char** decode(char* linea);
void execute_instruction(char** instruccion_a_ejecutar, contexto_ejecucion* ce);
void update_program_counter(contexto_ejecucion* ce);
void execute_process(contexto_ejecucion* ce);

/*-------------- MMU --------------------*/



int traducir_direccion_logica(int logical_address, contexto_ejecucion* ce, int valor_a_sumar);
char* fetch_value_in_memory(int physical_adress, contexto_ejecucion* ce, int size);
void store_value_in_register(char* register_mov_in, char* value);
char* encontrarValorDeRegistro(char* register_to_find_value);
void escribir_valor(int physical_address, char* register_value_mov_out, int segment_index, int pid);

int read_int(char* buffer, int* desp);

tamanio_registro(char* registro);

#endif