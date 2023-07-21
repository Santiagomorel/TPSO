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
    pthread_t threadKernel;

    pthread_create(&threadKernel, NULL, (void *) process_dispatch, NULL);
    pthread_join(threadKernel, NULL);
 
    

/*---------------------- TERMINO CPU ---------------------*/
	terminar_programa(cpu_logger, cpu_config_file);

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
        free(conexion_cpu);
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



void terminar_programa(t_log* logger, t_config* config)
{

	if(logger != NULL){
		log_destroy(logger);
	}

	if(config != NULL){
		config_destroy(config);
	}

	liberar_conexion(conexion_cpu);
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

		switch (op_code) {
            case EJECUTAR_CE: 
                contexto_ejecucion* ce = recibir_ce(socket_kernel);
                log_trace(cpu_logger, "Llego correctamente el CE con id: %d", ce->id);
                //imprimir_ce(ce, cpu_logger);
                execute_process(ce);
                break;   
            case -1:
                log_warning(cpu_logger, "El kernel se desconecto");
                end_cpu_module(1);
                pthread_exit(NULL);
                break;
            default:
                log_debug(cpu_logger, "Codigo de operacion desconocido");
                //exit(1);
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
t_segmento* segmento;
int sale_proceso = 0;
int offset = 0;
int direccion_logica;
int direccion_fisica;

void execute_instruction(char** instruction, contexto_ejecucion* ce){


    switch(keyfromstring(instruction[0])){
        case I_SET: 
            // SET (Registro, Valor)
            log_trace(cpu_logger, "Por ejecutar instruccion SET");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", ce->id, instruction[0], instruction[1], instruction[2]);

            sleep(atoi(cpu_config.retardo_instruccion)/1000);

            add_value_to_register(instruction[1], instruction[2]);

            save_context_ce(ce); // ACA GUARDAMOS EL CONTEXTO

            break;

        case I_IO:
            // I/O (Tiempo)
            log_trace(cpu_logger, "Por ejecutar instruccion I/O");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s", ce->id, instruction[0], instruction[1]);

            enviar_ce_con_string(socket_kernel, ce, instruction[1], BLOCK_IO);

            //input_ouput = 1;
            sale_proceso = 1;

            break;

         case I_EXIT:
            //EXIT: Esta instrucción representa la syscall de finalización del proceso.
            //Se deberá devolver el ce actualizado al Kernel para su finalización.
            log_trace(cpu_logger, "Instruccion EXIT ejecutada");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s", ce->id, instruction[0]);

            enviar_ce(socket_kernel, ce, SUCCESS, cpu_logger);

            //end_process = 1; // saca del while de ejecucion
            sale_proceso = 1;

            break;

        case I_WAIT:
            // WAIT (Recurso)
            //Esta instruccion asigna un recurso pasado por parametro
            log_trace(cpu_logger, "Por ejecutar instruccion WAIT");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s ", ce->id, instruction[0], instruction[1]);
            // Si rompe crear una varible char* recurso, asignandole instruccion[1] y enviar el recurso en el execute process
            
            enviar_ce_con_string(socket_kernel, ce, instruction[1], WAIT_RECURSO);

            //wait = 1;
            sale_proceso = 1;
            
            break;

        case I_SIGNAL:
            // SIGNAL (Recurso)
            //Esta instruccion libera un recurso pasado por parametro
            log_trace(cpu_logger, "Por ejecutar instruccion SIGNAL");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s", ce->id, instruction[0], instruction[1]);

            enviar_ce_con_string(socket_kernel, ce, instruction[1], SIGNAL_RECURSO);

            // signal_recurso = 1;
            sale_proceso = 1;
            
            break;
            
        case I_YIELD:
            log_trace(cpu_logger, "Por ejecutar instruccion YIELD");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s", ce->id, instruction[0]);
            
            enviar_ce(socket_kernel, ce, DESALOJO_YIELD, cpu_logger);

            // desalojo_por_yield = 1;
            sale_proceso = 1;

            break;

        case I_F_OPEN:
            log_trace(cpu_logger, "Por ejecutar instruccion F_OPEN");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s", ce->id, instruction[0], instruction[1]);

            enviar_ce_con_string(socket_kernel, ce, instruction[1], ABRIR_ARCHIVO);

            //desalojo_por_archivo = 1;
            sale_proceso = 1;

            break;

        case I_F_CLOSE:
            log_trace(cpu_logger, "Por ejecutar instruccion F_CLOSE");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s", ce->id, instruction[0], instruction[1]);

            
            enviar_ce_con_string(socket_kernel, ce, instruction[1], CERRAR_ARCHIVO);

            //desalojo_por_archivo = 1;
            sale_proceso = 1;

            break;

        case I_F_SEEK:
            log_trace(cpu_logger, "Por ejecutar instruccion F_SEEK");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s- %s", ce->id, instruction[0], instruction[1], instruction[2]);

            enviar_ce_con_string_entero(socket_kernel, ce, instruction[1], instruction[2], ACTUALIZAR_PUNTERO);
            //desalojo_por_archivo = 1;
            sale_proceso = 1;

            break;

        case I_F_READ:
            log_trace(cpu_logger, "Por ejecutar instruccion F_READ");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s - %s", ce->id, instruction[0], instruction[1], instruction[2], instruction[3]);

             
            direccion_logica = atoi(instruction[2]);
    
            direccion_fisica = traducir_direccion_logica(direccion_logica, ce, atoi(instruction[3]));

            log_trace(cpu_logger, "ya traduje a dir fisica y es :%d",direccion_fisica);
            
            
            enviar_ce_con_string_3_enteros(socket_kernel, ce, instruction[1], direccion_fisica, instruction[3], offset, LEER_ARCHIVO);

            //desalojo_por_archivo = 1;

            sale_proceso = 1;

            break;
        case I_F_WRITE:
            log_trace(cpu_logger, "Por ejecutar instruccion F_WRITE");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s - %s", ce->id, instruction[0], instruction[1], instruction[2], instruction[3]);
            
            direccion_logica = atoi(instruction[2]);
            direccion_fisica = traducir_direccion_logica(direccion_logica, ce, instruction[3]);

            enviar_ce_con_string_2_enteros(socket_kernel, ce, instruction[1], direccion_fisica, instruction[3], ESCRIBIR_ARCHIVO); 

            //desalojo_por_archivo = 1;
            sale_proceso = 1;

            break;
        case I_F_TRUNCATE:
            log_trace(cpu_logger, "Por ejecutar instruccion F_TRUNCATE");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", ce->id, instruction[0], instruction[1], instruction[2]);

            enviar_ce_con_string_entero(socket_kernel, ce, instruction[1], instruction[2], MODIFICAR_TAMAÑO_ARCHIVO);

            //desalojo_por_archivo = 1;
            sale_proceso = 1;

            break;

        case I_CREATE_SEGMENT:
            log_trace(cpu_logger, "Por ejecutar instruccion CREATE_SEGMENT");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", ce->id, instruction[0], instruction[1], instruction[2]);

            enviar_ce_con_dos_enteros(socket_kernel, ce, instruction[1], instruction[2], CREAR_SEGMENTO);

            //desalojo_por_archivo = 1;
            sale_proceso = 1;

            break;

        case I_DELETE_SEGMENT:
            log_trace(cpu_logger, "Por ejecutar instruccion DELETE_SEGMENT");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s", ce->id, instruction[0], instruction[1]);

            enviar_ce_con_entero(socket_kernel, ce, instruction[1], BORRAR_SEGMENTO);

            //desalojo_por_archivo = 1;
            sale_proceso = 1;
            
            break;

        case I_MOV_IN: //MOV_IN (Registro, Dirección Lógica)
            log_info(cpu_logger, "Instruccion MOV_IN ejecutada");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", ce->id, instruction[0], instruction[1], instruction[2]);

            char* register_mov_in = instruction[1];
            int logical_address_mov_in = atoi(instruction[2]);

            int size_movin = tamanio_registro(register_mov_in);    
            direccion_fisica = traducir_direccion_logica(logical_address_mov_in, ce, sizeof(register_mov_in));//fijarse si el sizeof(register_mov_in) es correcto

            
            //------------SI NO TENEMOS SEG FAULT EJECUTAMOS LO DEMAS ------------ //
            if(sigsegv != 1){
                
                char* value = fetch_value_in_memory(direccion_fisica, ce, size_movin);

                store_value_in_register(register_mov_in, value);
                log_info(cpu_logger, "PID: %d - Acción: LEER - Segmento: %d - Dirección Fisica: %d",
                        ce->id, segmento->id_segmento, direccion_fisica);
                    log_info(cpu_logger,"YA GUARDE VALOR EN REGISTRO!!");
            }

            break;

        case I_MOV_OUT: ///MOV_OUT (Dirección Lógica, Registro):
            log_info(cpu_logger, "Ejecutando Instruccion MOV_OUT ");
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", ce->id, instruction[0], instruction[1], instruction[2]);

            int logical_address_mov_out = atoi(instruction[1]);
            char* register_mov_out = instruction[2]; 
            
            int size_movout = tamanio_registro(register_mov_out);

            direccion_fisica = traducir_direccion_logica(logical_address_mov_out, ce, sizeof(register_mov_out));

            if(sigsegv != 1){
                 log_info(cpu_logger, "Recibimos una physical address valida!");
                char* register_value_mov_out = encontrarValorDeRegistro(register_mov_out);
                
                escribir_valor(direccion_fisica, register_value_mov_out, ce->id, size_movout);
                int code_op = recibir_operacion(conexion_cpu); // Si ta todo ok prosigo, si no ta todo ok que hago?

                if(code_op == MOV_OUT_OK) {  
                    log_info(cpu_logger,"Todo OK Prosiga");
                    
                    int size = 0;
                    int desp = 0;

                    void * buffer = recibir_buffer(&size, conexion_cpu);

                    log_info(cpu_logger, "Leo buffer: %d", leer_entero(buffer, &desp));
                    log_info(cpu_logger, "PID: %d - Acción: ESCRIBIR - Segmento: %d -  Dirección Fisica: %d", ce->id, segmento->id_segmento, direccion_fisica);
                }
                else {
                    log_error(conexion_cpu, "CODIGO DE OPERACION INVALIDO");
                }

            }
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

    //log_trace(cpu_logger, "Por empezar  end_process != 1 && input_ouput != 1 && wait != 0 && desalojo_por_yield != 1 && signal_recurso != 1 && sigsev != 1"); 
    log_trace(cpu_logger, "Comienza la ejecucion");
    // while(end_process != 1 && input_ouput != 1 && wait != 1 && desalojo_por_yield != 1 && signal_recurso != 1 && sigsegv != 1 && desalojo_por_archivo != 1 ){
    while(sale_proceso != 1){
        //Llega el ce y con el program counter buscas la instruccion que necesita
        instruction = string_duplicate(fetch_next_instruction_to_execute(ce));
        decoded_instruction = decode(instruction);

        log_trace(cpu_logger, "Por ejecutar la instruccion decodificada %s", decoded_instruction[0]);
        update_program_counter(ce);
        execute_instruction(decoded_instruction, ce);        
                
        log_trace(cpu_logger, "PROGRAM COUNTER: %d", ce->program_counter);

    } //si salis del while es porque te llego una interrupcion o termino el proceso o entrada y salida
    
    log_trace(cpu_logger, "SALI DEL WHILE DE EJECUCION");
    
    sale_proceso = 0;
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
    { "DELETE_SEGMENT", I_DELETE_SEGMENT },
    { "MOV_IN", I_MOV_IN },
    { "MOV_OUT", I_MOV_OUT }
};

int keyfromstring(char *key) {
    int i;
    for (i=0; i < 16; i++) {
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
    agregar_a_paquete(paquete, parameter, sizeof(parameter)+1);
    // agregar_string_a_paquete(paquete, parameter); 
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

void enviar_paquete_con_dos_enteros(int client_socket, char* x, char* y, int codOP){
    t_paquete* paquete = crear_paquete_op_code(codOP);

    agregar_entero_a_paquete(paquete, atoi(x)); 
    agregar_entero_a_paquete(paquete, atoi(y)); 
    enviar_paquete(paquete, client_socket);
    eliminar_paquete(paquete);
    
}

void enviar_ce_con_string_entero(int client_socket, contexto_ejecucion* ce, char* parameter, char* x, int codOP){
    t_paquete* paquete = crear_paquete_op_code(codOP);

    agregar_ce_a_paquete(paquete, ce, cpu_logger);
    log_error(cpu_logger,"el nombre del archivo es %s",parameter);
    agregar_string_a_paquete(paquete, parameter); 
    log_error(cpu_logger,"el entero del archivo es %s", x);
    agregar_entero_a_paquete(paquete, atoi(x));
    enviar_paquete(paquete, client_socket);
    eliminar_paquete(paquete);
    
}

void enviar_paquete_con_string_entero(int client_socket, char* parameter, char* x, int codOP){
    t_paquete* paquete = crear_paquete_op_code(codOP);

    agregar_string_a_paquete(paquete, parameter); 
    agregar_entero_a_paquete(paquete, atoi(x));
    enviar_paquete(paquete, client_socket);
    eliminar_paquete(paquete);
    
}


void enviar_ce_con_string_2_enteros(int client_socket, contexto_ejecucion* ce, char* parameter, char* x, char* y, int codOP){
    t_paquete* paquete = crear_paquete_op_code(codOP);

    agregar_ce_a_paquete(paquete, ce, cpu_logger);
    agregar_string_a_paquete(paquete, parameter); 
    agregar_entero_a_paquete(paquete, atoi(x));
    agregar_entero_a_paquete(paquete, atoi(y));
    enviar_paquete(paquete, client_socket);
    eliminar_paquete(paquete);
    
}
void enviar_ce_con_string_3_enteros(int client_socket, contexto_ejecucion* ce, char* parameter, int x, char* y, int z, int codOP){
    t_paquete* paquete = crear_paquete_op_code(codOP);

    agregar_ce_a_paquete(paquete, ce, cpu_logger);
    agregar_string_a_paquete(paquete, parameter); 
    agregar_entero_a_paquete(paquete, x);
    agregar_entero_a_paquete(paquete, atoi(y));
    agregar_entero_a_paquete(paquete, z);
    enviar_paquete(paquete, client_socket);
    eliminar_paquete(paquete);
    
}

void enviar_paquete_con_string_2_enteros(int client_socket, char* parameter, int x, char* y, int codOP){
    t_paquete* paquete = crear_paquete_op_code(codOP);

    agregar_string_a_paquete(paquete, parameter); 
    agregar_entero_a_paquete(paquete, x);
    agregar_entero_a_paquete(paquete, atoi(y));
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

/*---------------------------------- PARA MOV_OUT ----------------------------------*/

char* encontrarValorDeRegistro(char* register_to_find_value){ //Es t_registro* o char* ?
    if (strcmp(register_to_find_value, "AX") == 0) return registros->AX;
    else if (strcmp(register_to_find_value, "BX") == 0) return registros->BX;
    else if (strcmp(register_to_find_value, "CX") == 0) return registros->CX;
    else if (strcmp(register_to_find_value, "DX") == 0) return registros->DX;
    else if (strcmp(register_to_find_value, "EAX") == 0) return registros->EAX;
    else if (strcmp(register_to_find_value, "EBX") == 0) return registros->EBX;
    else if (strcmp(register_to_find_value, "ECX") == 0) return registros->ECX;
    else if (strcmp(register_to_find_value, "EDX") == 0) return registros->EDX;
    else if (strcmp(register_to_find_value, "RAX") == 0) return registros->RAX;
    else if (strcmp(register_to_find_value, "RBX") == 0) return registros->RBX;
    else if (strcmp(register_to_find_value, "RCX") == 0) return registros->RCX;
    else if (strcmp(register_to_find_value, "RDX") == 0) return registros->RDX;

    else return 0;
}

void escribir_valor(int physical_address, char* register_value_mov_out, int pid, int size){
    t_paquete* package = crear_paquete_op_code(MOV_OUT);
    agregar_entero_a_paquete(package, physical_address);
    agregar_string_a_paquete(package, register_value_mov_out);
    agregar_entero_a_paquete(package, pid);
    agregar_entero_a_paquete(package, size);
    enviar_paquete(package, conexion_cpu);
}
/*---------------------------------- MMU ----------------------------------*/


int traducir_direccion_logica(int logical_address, contexto_ejecucion* ce, int valor_a_sumar) {


    int num_segmento = (int) floor(logical_address / atoi(cpu_config.tam_max_segmento));
    int desplazamiento_segmento = logical_address % atoi(cpu_config.tam_max_segmento);

   
    offset = desplazamiento_segmento;
    log_error(cpu_logger,"desplazamiento segmento:%d",desplazamiento_segmento);


    t_list* segment_table_ce = ce->tabla_segmentos;

    log_trace(cpu_logger, "tamanio segment table es :%d",list_size(segment_table_ce));
    
    t_segmento* segment = list_get(segment_table_ce, num_segmento);

    log_trace(cpu_logger, "el id del segmento es :%d",segment->id_segmento);
    log_trace(cpu_logger, "la direccion base del segmento es :%d",segment->direccion_base);
    log_trace(cpu_logger, "el tamanio es :%d",segment->tamanio_segmento);

    /*segmento->id_segmento = num_segmento;
    log_trace(cpu_logger, "el id del segmento es :%d",segmento->id_segmento);
    segmento->tamanio_segmento = segment ->tamanio_segmento;
    segmento->direccion_base = desplazamiento_segmento + segment->direccion_base ;*/

    log_error(cpu_logger,"el calculo da: %d",desplazamiento_segmento+valor_a_sumar);
    if(desplazamiento_segmento + valor_a_sumar >= segment-> tamanio_segmento ){
        log_trace(cpu_logger, "entre en el if de segfault");
        desplazamiento_segfault = desplazamiento_segmento;
        log_trace(cpu_logger, "despues de desplazamiento segfault: %d",desplazamiento_segfault);
        id_segmento_con_segfault = segmento->id_segmento;
        log_trace(cpu_logger, "despues de segmento.idSegmento: %d",segmento->id_segmento);
        tamanio_segfault = segmento->tamanio_segmento;   /* Esta linea y la anterior es porque hay que imprimir cual fue el 
                                                            segmento que falló */

        sigsegv = 1;
    }
    log_trace(cpu_logger, "entre en el return de segfault");
      return (segment->direccion_base + desplazamiento_segmento);
}

char* fetch_value_in_memory(int physical_adress, contexto_ejecucion* ce, int size){

    t_paquete* package = crear_paquete_op_code(MOV_IN); 
    agregar_entero_a_paquete(package, ce->id);
    agregar_entero_a_paquete(package, physical_adress);
    agregar_entero_a_paquete(package, size);
    //agregar_ce_a_paquete(package,ce, cpu_logger);
    
    enviar_paquete(package, conexion_cpu);
    eliminar_paquete(package);
    log_info(cpu_logger, "MOV IN enviado");    

    int code_op = recibir_operacion(conexion_cpu);
    log_info(cpu_logger, "CODIGO OPERACION RECIBIDO EN CPU: %d", code_op);
    

    char* value_received;    
    int sizeb = 0, desp = 0;

    if(code_op == MOV_IN_OK) {  
        log_info(cpu_logger,"ENTRE CARAJO");
        void* buffer = recibir_buffer(&sizeb, conexion_cpu);
        value_received = leer_string(buffer, &desp); //AVERIGUAR BIEN ESTO
        log_info(conexion_cpu, "EL VALOR DEL REGISTRO RECIBIDO ES: %d", value_received);
    } else {
        log_error(conexion_cpu, "CODIGO DE OPERACION INVALIDO");
    }

    return value_received;
}



void store_value_in_register(char* register_mov_in, char* value){

    log_info(cpu_logger, "El registro %s quedara el valor: %d",register_mov_in ,value);

    add_value_to_register(register_mov_in, value);
}

/*---------------------------------- FUNCIONES EXTRAS ----------------------------------*/

int read_int(char* buffer, int* desp) {
	int ret;
	memcpy(&ret, buffer + (*desp), sizeof(int));
	(*desp)+=sizeof(int);
	return ret;
}
int tamanio_registro(char* registro){
    if (strcmp(registro, "AX") == 0) return 4;
    else if (strcmp(registro, "BX") == 0) return 4;
    else if (strcmp(registro, "CX") == 0) return 4;
    else if (strcmp(registro, "DX") == 0) return 4;
    else if (strcmp(registro, "EAX") == 0) return 8;
    else if (strcmp(registro, "EBX") == 0) return 8;
    else if (strcmp(registro, "ECX") == 0) return 8;
    else if (strcmp(registro, "EDX") == 0) return 8;
    else if (strcmp(registro, "RAX") == 0) return 16;
    else if (strcmp(registro, "RBX") == 0) return 16;
    else if (strcmp(registro, "RCX") == 0) return 16;
    else if (strcmp(registro, "RDX") == 0) return 16;
}