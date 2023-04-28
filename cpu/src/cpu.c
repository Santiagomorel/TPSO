#include <stdlib.h>
#include <stdio.h>
#include <commons/config.h>
#include <valgrind/valgrind.h>
#include <commons/log.h>
#include "cpu.h"

void funcion(char *str, int i) {
    VALGRIND_PRINTF_BACKTRACE("%s: %d\n", str, i);
}



int main(int argc, char ** argv) {



    /* ---------------- LOGGING ---------------- */

	cpu_logger = init_logger("./runlogs/cpu.log", "CPU", 1, LOG_LEVEL_TRACE);

	    log_info(cpu_logger, "Levanto la configuracion del cpu");
    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }

	/* ---------------- ARCHIVOS DE CONFIGURACION ---------------- */

	cpu_config_file = init_config(argv[1]);
	    if (cpu_config_file == NULL) {
        perror("Ocurrió un error al intentar abrir el archivo config");
        exit(1);
    }

    log_info(cpu_logger, "Cargo la configuracion de la cpu");
    
    load_config();
	
/*--------------------- CONECTO CON MEMORIA ----------------------*/

	establecer_conexion(cpu_config.ip_memoria, cpu_config.puerto_memoria, cpu_config_file, cpu_logger);

/*---------------------- CONEXION CON KERNEL ---------------------*/

	socket_cpu = iniciar_servidor(cpu_config.puerto_escucha, cpu_logger);
	esperar_cliente(socket_cpu);
	handshake_servidor(socket_cpu);

/*---------------------- TERMINO CPU ---------------------*/
	terminar_programa(conexion_cpu, cpu_logger, cpu_config_file);

	return 0;

}

void load_config(void){
	cpu_config.ip_memoria							= config_get_string_value(cpu_config_file, "IP_MEMORIA");
	cpu_config.puerto_memoria						= config_get_string_value(cpu_config_file, "PUERTO_MEMORIA");
	cpu_config.puerto_escucha						= config_get_string_value(cpu_config_file, "PUERTO_ESCUCHA");
	cpu_config.retardo_instruccion					= config_get_string_value(cpu_config_file, "RETARDO_INSTRUCCION");
	cpu_config.tam_max_segmento						= config_get_string_value(cpu_config_file, "TAM_MAX_SEGMENTO");

	log_info(cpu_logger, "Config cargada en 'cpu_cofig_file'");


}

void establecer_conexion(char * ip_memoria, char* puerto_memoria, t_config* config, t_log* logger){

	
	log_info(logger, "Inicio como cliente");

	log_info(logger,"Lei la IP %s , el Puerto Memoria %s ", ip_memoria, puerto_memoria);


	// Loggeamos el valor de config

    /* ---------------- LEER DE CONSOLA ---------------- */

	//leer_consola(logger);

	/*----------------------------------------------------------------------------------------------------------------*/

	// Creamos una conexión hacia el servidor
	conexion_cpu = crear_conexion(ip_memoria, puerto_memoria);

	// Enviamos al servidor el valor de ip como mensaje si es que levanta el cliente
	if((crear_conexion(ip_memoria, puerto_memoria)) == -1){
		log_info(logger, "Error al conectar con Memoria. El servidor no esta activo");
		exit(-1);
	}else{
		handshake_cliente(conexion_cpu);
		enviar_mensaje(ip_memoria, conexion_cpu);
	}



}

void handshake_servidor(int socket_cliente){
	uint32_t handshake;
	uint32_t resultOk = 0;
	uint32_t resultError = -1;

	recv(socket_cliente, &handshake, sizeof(uint32_t), MSG_WAITALL);
	if(handshake == 1)
   send(socket_cliente, &resultOk, sizeof(uint32_t), NULL);
	else
   send(socket_cliente, &resultError, sizeof(uint32_t), NULL);
}

void handshake_cliente(int socket_cliente){
	uint32_t handshake = 1;
	uint32_t result;

	send(socket_cliente, &handshake, sizeof(uint32_t), NULL);
	if(recv(socket_cliente, &result, sizeof(uint32_t), MSG_WAITALL) > 0){
		log_info(cpu_logger, "Establecí handshake con Memoria");
	}else{
		log_info(cpu_logger, "No pude establecer handshake con Memoria");
	}
}


void leer_consola(t_log* logger)
{
	
    char* leido;

 
    leido = readline("> ");

    while(strcmp(leido, "")) {
        log_info(logger, leido);
        leido = readline("> "); 
    }
	free(leido); 
}



void terminar_programa(int conexion, t_log* logger, t_config* config)
{

	if(logger != NULL){
		log_destroy(logger);
	}

	if(config != NULL){
		config_destroy(config);
	}

	liberar_conexion(conexion);
}

/*-------------------- REGISTROS -------------------*/
void set_registers(t_pcb* pcb) {
    registers[AX] = pcb->cpu_registers[AX];
    registers[BX] = pcb->cpu_registers[BX];
    registers[CX] = pcb->cpu_registers[CX];
    registers[DX] = pcb->cpu_registers[DX];
	registers[EAX] = pcb->cpu_registers[EAX];
	registers[EBX] = pcb->cpu_registers[EBX];
	registers[ECX] = pcb->cpu_registers[ECX];
	registers[EDX] = pcb->cpu_registers[EDX];
	registers[RAX] = pcb->cpu_registers[RAX];
	registers[RBX] = pcb->cpu_registers[RBX];
	registers[RCX] = pcb->cpu_registers[RCX];
	registers[RDX] = pcb->cpu_registers[RDX];


}

void init_registers() {
    registers[AX] = 0;
    registers[BX] = 0;
    registers[CX] = 0;
    registers[DX] = 0;
	registers[EAX] = 0;
    registers[EBX] = 0;
    registers[ECX] = 0;
    registers[EDX] = 0;
	registers[RAX] = 0;
    registers[RBX] = 0;
    registers[RCX] = 0;
    registers[RDX] = 0;
}

/*------------------- BUFFER --------------------*/
void* receive_buffer(int* size, int client_socket){
	void* buffer;
	recv(client_socket, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(client_socket, buffer, *size, MSG_WAITALL);
	return buffer;
}

/* ---------------- PCB ----------------*/

t_pcb* pcb_create(char** instructions, int client_socket, int pid) { // Para crear el PCB
    t_pcb* new_pcb = malloc(sizeof(t_pcb));

    new_pcb->process_id = pid;
    new_pcb->size = 0; // TODO: Ver en que basarse para ponele un size
    new_pcb->status = NEW;
	new_pcb->instructions = instructions;
    new_pcb->program_counter = 0;
    new_pcb->client_socket = client_socket;

    new_pcb->cpu_registers[AX] = 0;
	new_pcb->cpu_registers[BX] = 0;
	new_pcb->cpu_registers[CX] = 0;
	new_pcb->cpu_registers[DX] = 0;
	new_pcb->cpu_registers[EAX] = 0;
	new_pcb->cpu_registers[EBX] = 0;
	new_pcb->cpu_registers[ECX] = 0;
	new_pcb->cpu_registers[EDX] = 0;
	new_pcb->cpu_registers[RAX] = 0;
	new_pcb->cpu_registers[RBX] = 0;
	new_pcb->cpu_registers[RCX] = 0;
	new_pcb->cpu_registers[RDX] = 0;
 

    return new_pcb;
}
void save_context_pcb(t_pcb* pcb){
    pcb->cpu_registers[AX] = registers[AX];
    pcb->cpu_registers[BX] = registers[BX];
    pcb->cpu_registers[CX] = registers[CX];
    pcb->cpu_registers[DX] = registers[DX];
	pcb->cpu_registers[EAX] = registers[EAX];
	pcb->cpu_registers[EBX] = registers[EBX];
	pcb->cpu_registers[ECX] = registers[ECX];
	pcb->cpu_registers[EDX] = registers[EDX];
	pcb->cpu_registers[RAX] = registers[RAX];
	pcb->cpu_registers[RBX] = registers[RBX];
	pcb->cpu_registers[RCX] = registers[RCX];
	pcb->cpu_registers[RDX] = registers[RDX];

}


char* get_state_name(int state){
	char* string_state;
	switch(state){
		case NEW :
			string_state = string_duplicate("NEW");
			break;
		case READY:
			string_state = string_duplicate("READY");
			break;
		case BLOCKED:
			string_state = string_duplicate("BLOCKED");
			break;
		case RUNNING:
			string_state = string_duplicate("RUNNING");
			break;
		case EXIT:
			string_state = string_duplicate("EXIT");
			break;
		default:
			string_state = string_duplicate("ESTADO NO REGISTRADO");
			break;
	}
	return string_state;
}

void logguar_state(t_log* logger, int state) {
	char* string_state = get_state_name(state);
	log_info(logger, "Estado %d (%s)", state, string_state);
	free(string_state);
}


int read_int(char* buffer, int* desp) {
	int ret;
	memcpy(&ret, buffer + (*desp), sizeof(int));
	(*desp)+=sizeof(int);
	return ret;
}

/*---------------------- PARA INSTRUCCION SET -------------------*/

void add_value_to_register(char* registerToModify, char* valueToAdd){ 
    //convertir el valor a agregar a un tipo de dato int
    uint32_t value = atoi(valueToAdd);
    
    log_info(cpu_logger, "valor a sumarle al registro %d",value);
    if (strcmp(registerToModify, "AX") == 0) {
        registers[AX] += value;
    }
    else if (strcmp(registerToModify, "BX") == 0) {
        registers[BX] += value;
    }
    else if (strcmp(registerToModify, "CX") == 0) {
        registers[CX] += value;
    }
    else if (strcmp(registerToModify, "DX") == 0) {
        registers[DX] += value;
    }else if (strcmp(registerToModify, "EAX") == 0) {
        registers[EAX] += value;
    }else if (strcmp(registerToModify, "EBX") == 0) {
        registers[EBX] += value;
    }
    else if (strcmp(registerToModify, "ECX") == 0) {
        registers[ECX] += value;
    }
    else if (strcmp(registerToModify, "EDX") == 0) {
        registers[EDX] += value;
    }else if (strcmp(registerToModify, "RAX") == 0) {
        registers[RAX] += value;
    }else if (strcmp(registerToModify, "RBX") == 0) {
        registers[RBX] += value;
    }
    else if (strcmp(registerToModify, "RCX") == 0) {
        registers[RCX] += value;
    }
    else if (strcmp(registerToModify, "RDX") == 0) {
        registers[RDX] += value;
    }
}

/*-------------------- FETCH ---------------------- */
char* fetch_next_instruction_to_execute(t_pcb* pcb){
    return pcb->instructions[pcb->program_counter];
}




/*int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }

    t_config *config = config_create(argv[1]);
    if (config == NULL) {
        perror("Ocurrió un error al intentar abrir el archivo config");
        exit(1);
    }

    void print_key_and_value(char *key, void *value) {
        printf("%s => %s\n", key, (char *)value);
    }
    dictionary_iterator(config->properties, print_key_and_value);

    config_destroy(config);
    return 0;
}*/
