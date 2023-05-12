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
	esperar_cliente(socket_cpu, cpu_logger);
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
    registers[AX] = pcb->registros_cpu[AX];
    registers[BX] = pcb->registros_cpu[BX];
    registers[CX] = pcb->registros_cpu[CX];
    registers[DX] = pcb->registros_cpu[DX];
	registers[EAX] = pcb->registros_cpu[EAX];
	registers[EBX] = pcb->registros_cpu[EBX];
	registers[ECX] = pcb->registros_cpu[ECX];
	registers[EDX] = pcb->registros_cpu[EDX];
	registers[RAX] = pcb->registros_cpu[RAX];
	registers[RBX] = pcb->registros_cpu[RBX];
	registers[RCX] = pcb->registros_cpu[RCX];
	registers[RDX] = pcb->registros_cpu[RDX];


}

void init_registers() {
    registers[AX] = "";
    registers[BX] = "";
    registers[CX] = "";
    registers[DX] = "";
	registers[EAX] = "";
    registers[EBX] = "";
    registers[ECX] = "";
    registers[EDX] = "";
	registers[RAX] = "";
    registers[RBX] = "";
    registers[RCX] = "";
    registers[RDX] = "";
}



/* ---------------- PCB ----------------*/

void save_context_pcb(t_pcb* pcb){
    pcb->registros_cpu[AX] = registers[AX];
    pcb->registros_cpu[BX] = registers[BX];
    pcb->registros_cpu[CX] = registers[CX];
    pcb->registros_cpu[DX] = registers[DX];
	pcb->registros_cpu[EAX] = registers[EAX];
	pcb->registros_cpu[EBX] = registers[EBX];
	pcb->registros_cpu[ECX] = registers[ECX];
	pcb->registros_cpu[EDX] = registers[EDX];
	pcb->registros_cpu[RAX] = registers[RAX];
	pcb->registros_cpu[RBX] = registers[RBX];
	pcb->registros_cpu[RCX] = registers[RCX];
	pcb->registros_cpu[RDX] = registers[RDX];

}




/*---------------------- PARA INSTRUCCION SET -------------------*/

void add_value_to_register(char* registerToModify, char* valueToAdd){ 
    //convertir el valor a agregar a un tipo de dato int
 
    
    log_info(cpu_logger, "Caracteres a sumarle al registro %d",valueToAdd);
    if (strcmp(registerToModify, "AX") == 0) {
        registers[AX] = valueToAdd;
    }
    else if (strcmp(registerToModify, "BX") == 0) {
        registers[BX] = valueToAdd;
    }
    else if (strcmp(registerToModify, "CX") == 0) {
        registers[CX] = valueToAdd;
    }
    else if (strcmp(registerToModify, "DX") == 0) {
        registers[DX] = valueToAdd;
    }else if (strcmp(registerToModify, "EAX") == 0) {
        registers[EAX] = valueToAdd;
    }else if (strcmp(registerToModify, "EBX") == 0) {
        registers[EBX] = valueToAdd;
    }
    else if (strcmp(registerToModify, "ECX") == 0) {
        registers[ECX] = valueToAdd;
    }
    else if (strcmp(registerToModify, "EDX") == 0) {
        registers[EDX] = valueToAdd;
    }else if (strcmp(registerToModify, "RAX") == 0) {
        registers[RAX] = valueToAdd;
    }else if (strcmp(registerToModify, "RBX") == 0) {
        registers[RBX] = valueToAdd;
    }
    else if (strcmp(registerToModify, "RCX") == 0) {
        registers[RCX] = valueToAdd;
    }
    else if (strcmp(registerToModify, "RDX") == 0) {
        registers[RDX] = valueToAdd;
    }
}

/*-------------------- FETCH ---------------------- */
char* fetch_next_instruction_to_execute(t_pcb* pcb){
    return pcb->instrucciones[pcb->program_counter];
}

/*-------------------- DECODE ---------------------- */

char** decode(char* linea){ // separarSegunEspacios
    char** instruction = string_split(linea, " "); 

    if(instruction[0] == NULL){
        log_info(cpu_logger, "linea vacia!");
    }
    
    return instruction; 
}

/*-------------------- EXECUTE ---------------------- */

void execute_instruction(char** instruction, t_pcb* pcb){

     switch(keyfromstring(instruction[0])){
        case I_SET: 
            // SET (Registro, Valor)
            log_info(cpu_logger, "Por ejecutar instruccion SET");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", pcb->id, instruction[0], instruction[1], instruction[2]);

            usleep(atoi(cpu_config.retardo_instruccion));

            add_value_to_register(instruction[1], instruction[2]);
            break;
            default:
            log_info(cpu_logger, "No ejecute nada");
            break;
}
}

int end_process = 0;
int input_ouput = 0;
int check_interruption = 0;

char* device = "NONE";
char* parameter = "NONE";


void execute_process(t_pcb* pcb){
    //char* value_to_copy = string_new(); // ?????

    set_registers(pcb);

    char* instruction = malloc(sizeof(char*));
    char** decoded_instruction = malloc(sizeof(char*));

    log_info(cpu_logger, "Por empezar check_interruption != 1 && end_process != 1 && input_ouput != 1 && page_fault != 1"); 
    while(check_interruption != 1 && end_process != 1 && input_ouput != 1 && page_fault != 1 && sigsegv != 1){
        //Llega el pcb y con el program counter buscas la instruccion que necesita
        instruction = string_duplicate(fetch_next_instruction_to_execute(pcb));
        decoded_instruction = decode(instruction);

        log_info(cpu_logger, "Por ejecutar la instruccion decodificada %s", decoded_instruction[0]);
        execute_instruction(decoded_instruction, pcb);

        if(page_fault != 1) {   // en caso de tener page fault no se actualiza program counter
            update_program_counter(pcb);
        }
        
        
        log_info(cpu_logger, "PROGRAM COUNTER: %d", pcb->program_counter);


    } //si salis del while es porque te llego una interrupcion o termino el proceso o entrada y salida
    
    log_info(cpu_logger, "SALI DEL WHILE DE EJECUCION");


    save_context_pcb(pcb); // ACA GUARDAMOS EL CONTEXTO

   /* if(end_process) {
        end_process = 0; // IMPORTANTE: Apagar el flag para que no rompa el proximo proceso que llegue
        check_interruption = 0;
        send_pcb_package(socket_cpu, pcb, FIN_PROCESO);
        log_info(cpu_logger, "Enviamos paquete a dispatch: FIN PROCESO");
    } 
    else if(input_ouput) {
        input_ouput = 0;
        check_interruption = 0;
        log_info(cpu_logger, "Device: %s, Parameter: %s", device, parameter);
        send_pcb_io_package(socket_cpu, pcb, device, parameter, REQUEST); // Ver bien tema REQUEST
    }
    else if(page_fault) {
        page_fault = 0;
        check_interruption = 0;
        log_info(cpu_logger, "OCURRIO UN PAGE FAULT, ENVIANDO A KERNEL PARA QUE SOLUCIONE");
        
        // subirle pcb , 
        t_paquete* package = create_package(PAGE_FAULT); // FALTA PAGE_FAULT
        add_pcb_to_package(package, pcb);
        add_int_to_package(package, numeroSegmentoGlobalPageFault);
        add_int_to_package(package, numeroPaginaGlobalPageFault);
        send_package(package, socket_cpu);
        delete_package(package);
        //send_pcb_package(socket_kernel, pcb, REQUEST_PAGE_FAULT); //Este codigo de operacion?
    }
    else if(sigsegv == 1){
        sigsegv = 0;
        check_interruption = 0;
        log_info(cpu_logger, "Error: Segmentation Fault (SEG_FAULT), enviando para terminar proceso");
        send_pcb_package(socket_cpu, pcb, SEG_FAULT); //FALTA SEG_FAULT EN UTILS.H
    }
    else if(check_interruption) {
        check_interruption = 0;
        log_info(cpu_logger, "Entro por check interrupt");
        send_pcb_package(socket_cpu, pcb, EJECUTAR_INTERRUPCION); //Este codigo de operacion?
    }*/
}

/*---------------------------------- INSTRUCTIONS ----------------------------------*/

typedef struct { 
    char *key; 
    int val; 
    } t_symstruct;

static t_symstruct lookuptable[] = {
    { "SET", I_SET }	
};

int keyfromstring(char *key) {
    int i;
    for (i=0; i < 6; i++) {
        t_symstruct sym = lookuptable[i];
        if (strcmp(sym.key, key) == 0)
            return sym.val;
    }
    return BADKEY;
}


void update_program_counter(t_pcb* pcb){
    pcb->program_counter += 1;
}