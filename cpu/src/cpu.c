#include "cpu.h"

void funcion(char *str, int i) {
    VALGRIND_PRINTF_BACKTRACE("%s: %d\n", str, i);
}

void end_cpu_module(sig_t s){
    close(socket_cpu);
    close(conexion_cpu);
    exit(1); 
}

int main(int argc, char ** argv) {

    signal(SIGINT, end_cpu_module);

    /* ---------------- LOGGING ---------------- */
	cpu_logger = init_logger("./runlogs/cpu.log", "CPU", 1, LOG_LEVEL_TRACE);

    log_info(cpu_logger, "INICIA EL MODULO DE CPU");
	
    log_trace(cpu_logger, "Levanto la configuracion del cpu");
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

    log_trace(cpu_logger, "Cargo la configuracion de la cpu");
    
    load_config();
	
/*--------------------- CONECTO CON MEMORIA ----------------------*/

	establecer_conexion(cpu_config.ip_memoria, cpu_config.puerto_memoria, cpu_config_file, cpu_logger);
    
/*---------------------- CONEXION CON KERNEL ---------------------*/
/*
	socket_cpu = iniciar_servidor(cpu_config.puerto_escucha, cpu_logger);
	esperar_cliente(socket_cpu, cpu_logger);
	handshake_servidor(socket_cpu);
*/
    pthread_t threadDispatch;

    pthread_create(&threadDispatch, NULL, (void *) process_dispatch, NULL);
    pthread_join(threadDispatch, NULL);
 
    

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

	log_trace(cpu_logger, "Config cargada en 'cpu_cofig_file'");


}

void establecer_conexion(char * ip_memoria, char* puerto_memoria, t_config* config, t_log* logger){

	
	log_trace(logger, "Inicio como cliente");

	log_trace(logger,"Lei la IP %s , el Puerto Memoria %s ", ip_memoria, puerto_memoria);

	/*----------------------------------------------------------------------------------------------------------------*/

	// Enviamos al servidor el valor de ip como mensaje si es que levanta el cliente
	if((conexion_cpu = crear_conexion(ip_memoria, puerto_memoria)) == -1){
		log_trace(logger, "Error al conectar con Memoria. El servidor no esta activo");
		exit(-1);
	}else{
		//handshake_cliente(conexion_cpu);
		enviar_mensaje(ip_memoria, conexion_cpu);
	}

    recibir_operacion(conexion_cpu);
    recibir_mensaje(conexion_cpu, cpu_logger);


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
		log_trace(cpu_logger, "Establecí handshake con Memoria");
	}else{
		log_trace(cpu_logger, "No pude establecer handshake con Memoria");
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


/*-------------------- HILOS -------------------*/
void process_dispatch() {
    log_trace(cpu_logger, "Soy el proceso Dispatch");
    socket_cpu = iniciar_servidor(cpu_config.puerto_escucha, cpu_logger);
	log_trace(cpu_logger, "Servidor DISPATCH listo para recibir al cliente");

	socket_kernel = esperar_cliente(socket_cpu, cpu_logger);
    enviar_mensaje("Dispatch conectado",socket_kernel);
    //handshake_servidor(socket_kernel);
    
    log_trace(cpu_logger, "Esperando a que envie mensaje/paquete");

    // sem_wait(mutex_cpu);
	while (1) {
		int op_code = recibir_operacion(socket_kernel);
        log_trace(cpu_logger, "Codigo de operacion recibido de kernel: %d", op_code);
        contexto_ejecucion* ce; //hay que hacer un free del contexto de ejecucion una vez termine de ejecutar

		switch (op_code) {
            case EJECUTAR_CE: 
                ce = recibir_ce(socket_kernel);
                log_trace(cpu_logger, "Llego correctamente el CE con id: %d", ce->id);
                imprimir_ce(ce, cpu_logger);
                execute_process(ce);
                break;   
            case -1:
                log_warning(cpu_logger, "El kernel se desconecto");
                end_cpu_module(1);
                pthread_exit(NULL);
                break;
            default:
                log_error(cpu_logger, "Codigo de operacion desconocido");
                exit(1);
                break;     
	    }
    }
}




/*-------------------- REGISTROS -------------------*/
void set_registers(contexto_ejecucion* ce) {
    registros = malloc(sizeof(t_registro)); // ATENCION (metrovias informa) hace falta un free supongo despues de reenviar el ce
    strncpy(registros->AX, ce->registros_cpu->AX, 4);
    strncpy(registros->BX , ce->registros_cpu->BX, 4);
    strncpy(registros->CX , ce->registros_cpu->CX, 4);
    strncpy(registros->DX , ce->registros_cpu->DX, 4);
	strncpy(registros->EAX , ce->registros_cpu->EAX, 8);
	strncpy(registros->EBX , ce->registros_cpu->EBX, 8);
	strncpy(registros->ECX , ce->registros_cpu->ECX, 8);
	strncpy(registros->EDX , ce->registros_cpu->EDX, 8);
	strncpy(registros->RAX , ce->registros_cpu->RAX, 16);
	strncpy(registros->RBX , ce->registros_cpu->RBX, 16);
	strncpy(registros->RCX , ce->registros_cpu->RCX, 16);
	strncpy(registros->RDX , ce->registros_cpu->RDX, 16);

}



/* ---------------- Contexto Ejecucion ----------------*/

void save_context_ce(contexto_ejecucion* ce){

    strncpy(ce->registros_cpu->AX, registros->AX, 4);   
    strncpy(ce->registros_cpu->BX, registros->BX, 4);   
    strncpy(ce->registros_cpu->CX, registros->CX, 4);  
    strncpy(ce->registros_cpu->DX, registros->DX, 4); 
    strncpy(ce->registros_cpu->EAX ,registros->EAX, 8);
	strncpy(ce->registros_cpu->EBX ,registros->EBX, 8);
	strncpy(ce->registros_cpu->ECX ,registros->ECX, 8);
	strncpy(ce->registros_cpu->EDX ,registros->EDX, 8);
	strncpy(ce->registros_cpu->RAX ,registros->RAX, 16);
	strncpy(ce->registros_cpu->RBX ,registros->RBX, 16);
	strncpy(ce->registros_cpu->RCX ,registros->RCX, 16);
	strncpy(ce->registros_cpu->RDX ,registros->RDX, 16);

}




/*---------------------- PARA INSTRUCCION SET -------------------*/

void add_value_to_register(char* registerToModify, char* valueToAdd){ 
    //convertir el valor a agregar a un tipo de dato int
 
    
    log_trace(cpu_logger, "Caracteres a sumarle al registro %s", valueToAdd);
    if (strcmp(registerToModify, "AX") == 0) {
        strncpy(registros->AX , valueToAdd, 4);
    }
    else if (strcmp(registerToModify, "BX") == 0) {
        strncpy(registros->BX , valueToAdd, 4);
    }
    else if (strcmp(registerToModify, "CX") == 0) {
        strncpy(registros->CX , valueToAdd, 4);
    }
    else if (strcmp(registerToModify, "DX") == 0) {
        strncpy(registros->DX , valueToAdd, 4);
    }
    else if (strcmp(registerToModify, "EAX") == 0) {
        strncpy(registros->EAX , valueToAdd, 8);
    }
    else if (strcmp(registerToModify, "EBX") == 0) {
        strncpy(registros->EBX , valueToAdd, 8);
    }
    else if (strcmp(registerToModify, "ECX") == 0) {
        strncpy(registros->ECX , valueToAdd, 8);
    }
    else if (strcmp(registerToModify, "EDX") == 0) {
        strncpy(registros->EDX , valueToAdd, 8);
    }
    else if (strcmp(registerToModify, "RAX") == 0) {
        strncpy(registros->RAX , valueToAdd, 16);
    }
    else if (strcmp(registerToModify, "RBX") == 0) {
        strncpy(registros->RBX , valueToAdd, 16);
    }
    else if (strcmp(registerToModify, "RCX") == 0) {
        strncpy(registros->RCX , valueToAdd, 16);
    }
    else if (strcmp(registerToModify, "RDX") == 0) {
        strncpy(registros->RDX , valueToAdd, 16);
    }
}

/*-------------------- FETCH ---------------------- */
char* fetch_next_instruction_to_execute(contexto_ejecucion* ce){
    return ce->instrucciones[ce->program_counter];
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

int end_process = 0;
int input_ouput = 0;
int check_interruption = 0;
int wait = 0;
int desalojo_por_yield = 0;
int signal_recurso = 0;


char* tiempo = "NONE";

void execute_instruction(char** instruction, contexto_ejecucion* ce){

     switch(keyfromstring(instruction[0])){
        case I_SET: 
            // SET (Registro, Valor)
            log_trace(cpu_logger, "Por ejecutar instruccion SET");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", ce->id, instruction[0], instruction[1], instruction[2]);

            sleep(atoi(cpu_config.retardo_instruccion)/1000);

            add_value_to_register(instruction[1], instruction[2]);
            break;
        case I_IO:
            // I/O (Tiempo)
            log_trace(cpu_logger, "Por ejecutar instruccion I/O");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s", ce->id, instruction[0], instruction[1]);

            tiempo = instruction[1];
            enviar_ce_con_entero(socket_kernel, ce, tiempo, BLOCK_IO);
            log_trace(cpu_logger, "%s", tiempo);
            input_ouput = 1;
            break;
         case I_EXIT:
            //EXIT: Esta instrucción representa la syscall de finalización del proceso.
            //Se deberá devolver el ce actualizado al Kernel para su finalización.
            log_trace(cpu_logger, "Instruccion EXIT ejecutada");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s", ce->id, instruction[0]);
            
            end_process = 1;
            break;
        case I_WAIT:
            // WAIT (Recurso)
            //Esta instruccion asigna un recurso pasado por parametro
            log_trace(cpu_logger, "Por ejecutar instruccion WAIT");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s ", ce->id, instruction[0], instruction[1]);
            // Si rompe crear una varible char* recurso, asignandole instruccion[1] y enviar el recurso en el execute process
            
            enviar_ce_con_string(socket_kernel, ce, instruction[1], WAIT_RECURSO);

            wait = recibir_respuesta_recurso();
            
            break;
        case I_SIGNAL:
            // SIGNAL (Recurso)
            //Esta instruccion libera un recurso pasado por parametro
            log_trace(cpu_logger, "Por ejecutar instruccion SIGNAL");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s", ce->id, instruction[0], instruction[1]);

            enviar_ce_con_string(socket_kernel, ce, instruction[1], SIGNAL_RECURSO);

            signal_recurso = recibir_respuesta_recurso();

            
            break;
        case I_YIELD:
            log_trace(cpu_logger, "Por ejecutar instruccion YIELD");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s", ce->id, instruction[0]);
            
            desalojo_por_yield = 1;
            break;
        case I_F_OPEN:
        log_trace(cpu_logger, "Por ejecutar instruccion F_OPEN");
        log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s", ce->id, instruction[0], instruction[1]);

        enviar_ce_con_string(socket_kernel, ce, instruction[1], ABRIR_ARCHIVO);
        
            break;
        case I_F_CLOSE:
        log_trace(cpu_logger, "Por ejecutar instruccion F_CLOSE");
        log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s", ce->id, instruction[0], instruction[1]);

        
        enviar_ce_con_string(socket_kernel, ce, instruction[1], CERRAR_ARCHIVO);

            break;
        case I_F_SEEK:
        log_trace(cpu_logger, "Por ejecutar instruccion F_SEEK");
        log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s- %s", ce->id, instruction[0], instruction[1], instruction[2]);

        enviar_ce_con_string_entero(socket_kernel, ce, instruction[1], instruction[2], ACTUALIZAR_PUNTERO);

            break;
        case I_F_READ:
        log_trace(cpu_logger, "Por ejecutar instruccion F_READ");
        log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", ce->id, instruction[0], instruction[1], instruction[2]);
            break;
        case I_F_WRITE:
        log_trace(cpu_logger, "Por ejecutar instruccion F_WRITE");
        log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", ce->id, instruction[0], instruction[1], instruction[2]);
            break;
        case I_F_TRUNCATE:
        log_trace(cpu_logger, "Por ejecutar instruccion F_TRUNCATE");
        log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", ce->id, instruction[0], instruction[1], instruction[2]);

        enviar_ce_con_string_entero(socket_kernel, ce, instruction[1], instruction[2], MODIFICAR_TAMAÑO_ARCHIVO);

            break;
        case I_CREATE_SEGMENT:
        log_trace(cpu_logger, "Por ejecutar instruccion CREATE_SEGMENT");
        log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", ce->id, instruction[0], instruction[1], instruction[2]);

        enviar_ce_con_dos_enteros(socket_kernel, ce, instruction[1], instruction[2], CREAR_SEGMENTO);

            break;
        case I_DELETE_SEGMENT:
        log_trace(cpu_logger, "Por ejecutar instruccion DELETE_SEGMENT");
        log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s", ce->id, instruction[0], instruction[1]);

        enviar_ce_con_entero(socket_kernel, ce, instruction[1], BORRAR_SEGMENTO);

            break;
        default:
            log_info(cpu_logger, "No ejecute nada");
            break;
}
}




void execute_process(contexto_ejecucion* ce){
    //char* value_to_copy = string_new(); // ?????
    set_registers(ce);
    log_trace(cpu_logger, "Seteo los registros");

    char* instruction = malloc(sizeof(char*));
    char** decoded_instruction = malloc(sizeof(char*));

    log_trace(cpu_logger, "Por empezar check_interruption != 1 && end_process != 1 && input_ouput != 1 && wait == 0 && desalojo_por_yield != 1"); 
    while(check_interruption != 1 && end_process != 1 && input_ouput != 1 && wait == 0 && desalojo_por_yield != 1 && signal_recurso != 2){
        //Llega el ce y con el program counter buscas la instruccion que necesita
        instruction = string_duplicate(fetch_next_instruction_to_execute(ce));
        decoded_instruction = decode(instruction);

        log_trace(cpu_logger, "Por ejecutar la instruccion decodificada %s", decoded_instruction[0]);
        execute_instruction(decoded_instruction, ce);

        if(page_fault != 1) {   // en caso de tener page fault no se actualiza program counter
            update_program_counter(ce);
        }
        
        
        log_trace(cpu_logger, "PROGRAM COUNTER: %d", ce->program_counter);


    } //si salis del while es porque te llego una interrupcion o termino el proceso o entrada y salida
    
    log_trace(cpu_logger, "SALI DEL WHILE DE EJECUCION");


    save_context_ce(ce); // ACA GUARDAMOS EL CONTEXTO
    imprimir_registros(ce->registros_cpu, cpu_logger); // para comprobar que los registros se guardaran bien
    if(end_process) {
        end_process = 0; // IMPORTANTE: Apagar el flag para que no rompa el proximo proceso que llegue
        check_interruption = 0;
        enviar_ce(socket_kernel, ce, SUCCESS, cpu_logger);
        liberar_ce(ce);
        log_trace(cpu_logger, "Enviamos paquete a dispatch: FIN PROCESO");
    } 
    else if(input_ouput) {
        input_ouput = 0;
        check_interruption = 0;
        log_trace(cpu_logger, "Tiempo: %s", tiempo);
 
    }
    /*else if(page_fault) {
        page_fault = 0;
        check_interruption = 0;
        log_info(cpu_logger, "OCURRIO UN PAGE FAULT, ENVIANDO A KERNEL PARA QUE SOLUCIONE");
        
        // subirle ce , 
        t_paquete* package = create_package(PAGE_FAULT); // FALTA PAGE_FAULT
        add_ce_to_package(package, ce);
        add_int_to_package(package, numeroSegmentoGlobalPageFault);
        add_int_to_package(package, numeroPaginaGlobalPageFault);
        send_package(package, socket_cpu);
        delete_package(package);
        //send_ce_package(socket_kernel, ce, REQUEST_PAGE_FAULT); //Este codigo de operacion?
    }*/
    /*else if(sigsegv == 1){
        sigsegv = 0;
        check_interruption = 0;
        log_info(cpu_logger, "Error: Segmentation Fault (SEG_FAULT), enviando para terminar proceso");
        send_ce_package(socket_cpu, ce, SEG_FAULT); //FALTA SEG_FAULT EN UTILS.H
    }*/
    else if(check_interruption) {
        check_interruption = 0;
        log_trace(cpu_logger, "Entro por check interrupt");
        enviar_ce(socket_kernel, ce, EJECUTAR_INTERRUPCION, cpu_logger); 
    }else if(wait){
       
       
        if(wait == 1){  // Se bloquea por estar ocupado recurso
            log_trace(cpu_logger, "Bloqueado por WAIT");
            enviar_ce(socket_kernel, ce, BLOCK_WAIT, cpu_logger);
            liberar_ce(ce);
        }else{
            log_trace(cpu_logger, "No existe el recurso");
            enviar_ce(socket_kernel, ce, EXIT_RECURSO, cpu_logger);
            liberar_ce(ce);
        }
             
        
        wait = 0;
        liberar_ce(ce);
    }else if(desalojo_por_yield){
        desalojo_por_yield = 0;
        log_trace(cpu_logger, "Desalojado por YIELD");
        enviar_ce(socket_kernel, ce, DESALOJO_YIELD, cpu_logger);
        liberar_ce(ce);
    }else if(signal_recurso){
        log_trace(cpu_logger, "No existe el recurso");
        enviar_ce(socket_kernel, ce, EXIT_RECURSO, cpu_logger);
        liberar_ce(ce);
    }
}

int recibir_respuesta_recurso(){
    int codigo_op = recibir_operacion(socket_kernel);

    if(codigo_op == NO_EXISTE_RECURSO){
        return 2;
    }else if (codigo_op == NO_LO_TENGO){
        return 1;
    }else if(codigo_op == LO_TENGO){
        return 0;
    }
}

/*---------------------------------- INSTRUCTIONS ----------------------------------*/

typedef struct { 
    char *key; 
    int val; 
    } t_symstruct;

static t_symstruct lookuptable[] = {
    { "SET", I_SET },
    { "I/O", I_IO },
    { "EXIT", I_EXIT },
    { "WAIT", I_WAIT },
    { "SIGNAL", I_SIGNAL },
    { "YIELD",I_YIELD},
    { "F_OPEN", I_F_OPEN },
    { "F_CLOSE", I_F_CLOSE },
    { "F_SEEK", I_F_SEEK },
    { "F_READ", I_F_READ },
    { "F_WRITE", I_F_WRITE },
    { "F_TRUNCATE",I_F_TRUNCATE},
    { "CREATE_SEGMENT", I_CREATE_SEGMENT },
    { "DELETE_SEGMENT", I_DELETE_SEGMENT }
};

int keyfromstring(char *key) {
    int i;
    for (i=0; i < 14; i++) {
        t_symstruct sym = lookuptable[i];
        if (strcmp(sym.key, key) == 0)
            return sym.val;
    }
    return BADKEY;
}


void update_program_counter(contexto_ejecucion* ce){
    ce->program_counter += 1;
}

/*---------------------------------- PARA INSTRUCCION WAIT Y SIGNAL ----------------------------------*/

void enviar_ce_con_string(int client_socket, contexto_ejecucion* ce, char* parameter, int codOP){
    t_paquete* paquete = crear_paquete_op_code(codOP);

    agregar_ce_a_paquete(paquete, ce, cpu_logger);
    agregar_string_a_paquete(paquete, parameter); 
    enviar_paquete(paquete, client_socket);
    eliminar_paquete(paquete);
    
}



void enviar_ce_con_dos_enteros(int client_socket, contexto_ejecucion* ce, char* x, char* y, int codOP){
    t_paquete* paquete = crear_paquete_op_code(codOP);

    agregar_ce_a_paquete(paquete, ce, cpu_logger);
    agregar_entero_a_paquete(paquete, atoi(x)); 
    agregar_entero_a_paquete(paquete, atoi(y)); 
    enviar_paquete(paquete, client_socket);
    eliminar_paquete(paquete);
    
}

void enviar_ce_con_string_entero(int client_socket, contexto_ejecucion* ce, char* parameter, char* x, int codOP){
    t_paquete* paquete = crear_paquete_op_code(codOP);

    agregar_ce_a_paquete(paquete, ce, cpu_logger);
    agregar_string_a_paquete(paquete, parameter); 
    agregar_entero_a_paquete(paquete, atoi(x));
    enviar_paquete(paquete, client_socket);
    eliminar_paquete(paquete);
    
}




/*---------------------------------- PARA INSTRUCCION IO ----------------------------------*/

void enviar_ce_con_entero(int client_socket, contexto_ejecucion* ce, char* x, int codOP){
    t_paquete* paquete = crear_paquete_op_code(codOP);

    agregar_ce_a_paquete(paquete, ce, cpu_logger);
    agregar_entero_a_paquete(paquete, atoi(x));
    enviar_paquete(paquete, client_socket);
    eliminar_paquete(paquete);
}
