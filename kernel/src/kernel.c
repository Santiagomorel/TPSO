#include "kernel.h"

int main(int argc, char ** argv)
{
    // ----------------------- creo el log del kernel ----------------------- //

    kernel_logger = init_logger("./runlogs/kernel.log", "KERNEL", 1, LOG_LEVEL_TRACE);

    // ----------------------- levanto y cargo la configuracion del kernel ----------------------- //

    log_trace(kernel_logger, "levanto la configuracion del kernel");
    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }
    
    kernel_config_file = init_config(argv[1]);

    if (kernel_config_file == NULL) {
        perror("Ocurrió un error al intentar abrir el archivo config");
        exit(1);
    }

    log_trace(kernel_logger, "cargo la configuracion del kernel");
    
    load_config();

    inicializarListasGlobales();

    iniciarSemaforos();

    iniciar_planificadores();

    // ----------------------- contecto el kernel con los servidores de MEMORIA - CPU (dispatch) - FILESYSTEM ----------------------- //

    // if((memory_connection = crear_conexion(kernel_config.ip_memoria , kernel_config.puerto_memoria)) == -1) {
    //     log_trace(kernel_logger, "No se pudo conectar al servidor de MEMORIA");
    //     exit(2);
    // }

    // if((cpu_dispatch_connection = crear_conexion(kernel_config.ip_cpu , kernel_config.puerto_cpu)) == -1) {
    //     log_trace(kernel_logger, "No se pudo conectar al servidor de CPU DISPATCH");
    //     exit(2);
    // }

    // if((file_system_connection = crear_conexion(kernel_config.ip_file_system , kernel_config.puerto_file_system)) == -1) {
    //     log_trace(kernel_logger, "No se pudo conectar al servidor de FILE SYSTEM");
    //     exit(2);
    // }

    // ----------------------- levanto el servidor del kernel ----------------------- //

    socket_servidor_kernel = iniciar_servidor(kernel_config.puerto_escucha, kernel_logger);
    log_trace(kernel_logger, "kernel inicia el servidor");

    // ----------------------- espero conexiones de consola ----------------------- //

    while (1) {

        log_trace(kernel_logger, "esperando cliente consola");
	    socket_cliente = esperar_cliente(socket_servidor_kernel, kernel_logger);
            log_trace(kernel_logger, "entro una consola con el socket: %d", socket_cliente); 

    
        pthread_t atiende_consola;
            pthread_create(&atiende_consola, NULL, (void*)recibir_consola, (void*)socket_cliente);
            pthread_detach(atiende_consola);

    }

    // end_program(0/*cambiar por conexion*/, kernel_logger, kernel_config_file);
    return EXIT_SUCCESS;
}

// ----------------------- Funciones Globales de Kernel ----------------------- //

// void iterator(char* value) {
// 	log_trace(kernel_logger,"%s", value);
// }

void load_config(void){

    kernel_config.ip_memoria                   = config_get_string_value(kernel_config_file, "IP_MEMORIA");
    kernel_config.puerto_memoria               = config_get_string_value(kernel_config_file, "PUERTO_MEMORIA");
    kernel_config.ip_file_system               = config_get_string_value(kernel_config_file, "IP_FILESYSTEM");
    kernel_config.puerto_file_system           = config_get_string_value(kernel_config_file, "PUERTO_FILESYSTEM");
    kernel_config.ip_cpu                       = config_get_string_value(kernel_config_file, "IP_CPU");
    kernel_config.puerto_cpu                   = config_get_string_value(kernel_config_file, "PUERTO_CP");
    kernel_config.puerto_escucha               = config_get_string_value(kernel_config_file, "PUERTO_ESCUCHA");
    kernel_config.algoritmo_planificacion      = config_get_string_value(kernel_config_file, "ALGORITMO_CLASIFICACION");

    kernel_config.estimacion_inicial           = config_get_int_value(kernel_config_file, "ESTIMACION_INICIAL");

    kernel_config.hrrn_alfa                    = config_get_long_value(kernel_config_file, "HRRN_ALFA");
    
    kernel_config.grado_max_multiprogramacion  = config_get_int_value(kernel_config_file, "GRADO_MAX_MULTIPROGRAMACION");
    
    kernel_config.recursos                     = config_get_string_value(kernel_config_file, "RECURSOS");
    kernel_config.instancias_recursos          = config_get_string_value(kernel_config_file, "INSTANCIAS_RECURSOS");
    
    // kernel_config.ip_kernel                    = config_get_string_value(kernel_config_file, "IP_KERNEL");
    // kernel_config.puerto_kernel                = config_get_string_value(kernel_config_file, "PUERTO_KERNEL");
    
    log_trace(kernel_logger, "config cargada en 'kernel_cofig_file'");
}

// void end_program(int socket, t_log* log, t_config* config){
//     log_destroy(log);
//     config_destroy(config);
//     liberar_conexion(socket);
// }

void recibir_consola(int SOCKET_CLIENTE) {
    int codigoOperacion = recibir_operacion(SOCKET_CLIENTE);

        switch(codigoOperacion)
        {
            case MENSAJE:
                log_trace(kernel_logger, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
            
                break;
            // ---------LP entrante----------
            case INICIAR_PCB:

            log_trace(kernel_logger, "consola envia pseudocodigo, inicio PCB");
            t_pcb* pcb_a_iniciar = iniciar_pcb(SOCKET_CLIENTE);
            log_trace(kernel_logger, "pcb iniciado PID : %d", pcb_a_iniciar->id);
            pthread_mutex_lock(&m_listaNuevos);
                list_add(listaNuevos, pcb_a_iniciar);
            pthread_mutex_unlock(&m_listaNuevos);
            log_info(kernel_logger, "Se crea el proceso %d en NEW", pcb_a_iniciar->id);

            enviar_mensaje("Llego el paquete",SOCKET_CLIENTE);

            // si multprog permite -> pido tabla segmentos memoria
            // me interesa comprobar la multiprogramacion, de momento
            

            //enviar_Fin_consola(SOCKET_CLIENTE);
            //planificar_sig_to_ready(); // usar esta funcion cada vez q se agregue un proceso a NEW o SUSPENDED-BLOCKED 
            break;

            default:
                log_trace(kernel_logger, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
                break;
        }
}

t_pcb* iniciar_pcb(int socket) {
    int size = 0;

    char* buffer;
    int desp = 0;
    
    buffer = recibir_buffer(&size, socket);
    
    char* instrucciones = leer_string(buffer, &desp);
    // int tamanio_proceso = leer_entero(buffer, &desp);        //TODO aca tengo que leer la cantidad de lineas de pseudocodigo
    
    t_pcb* nuevo_pcb = pcb_create(instrucciones, tamanio_proceso, socket); 
    free(instrucciones);
    free(buffer);
    
    loggear_pcb(nuevo_pcb, kernel_logger);
    
    return nuevo_pcb;
}

t_pcb* pcb_create(char* instrucciones, int tamanio, int socket_consola) {
    log_trace(kernel_logger, "instrucciones en pcb create %s", instrucciones);
    t_pcb* new_pcb = malloc(sizeof(t_pcb));

    generar_id(new_pcb);
    new_pcb->estado_actual = NEW;
    new_pcb->tamanio = tamanio;
    //list_add_all(new_pcb->instrucciones, instrucciones); // La lista del PCB se la tendre que pasar como & ?
    //printf("%s" ,string_duplicate(instrucciones));
    new_pcb->instrucciones = separar_inst_en_lineas(instrucciones);
    new_pcb->program_counter = 0;
    // new_pcb->tabla_paginas = -1; // TODO de la conexion con memoria
    new_pcb->estimacion_rafaga = kernel_config.estimacion_inicial; //ms 
    new_pcb->estimacion_fija = kernel_config.estimacion_inicial; // ms
    new_pcb->rafaga_anterior = 0.0; 
    new_pcb->socket_consola = socket_consola;
    // new_pcb->sumatoria_rafaga = 0.0;
    
    return new_pcb;
}

void generar_id(t_pcb* pcb) { // Con esta funcion le asignamos a un pcb su id, cuidando de no repetir el numero
        pthread_mutex_lock(&m_contador_id);
    pcb->id = contador_id;
    contador_id++;
        pthread_mutex_unlock(&m_contador_id);
}

char** separar_inst_en_lineas(char* instrucciones) {
    char** lineas = parsearPorSaltosDeLinea(instrucciones);

    char** lineas_mejorado = string_array_new();
    int i; 

    for(i = 0; i < string_array_size(lineas); i++){
        
        char** instruccion = string_split(lineas[i], " "); // separo por espacios 
        log_trace(kernel_logger, "Agrego la linea '%s' a las intrucciones del pcb", lineas[i]);
        //log_info(logger_kernel, "instruccion[0] es: %s y la instruccion[1] es:%s", instruccion[0], instruccion[1]);
        string_array_push(&lineas_mejorado, string_duplicate(lineas[i]));
    }
    string_array_destroy(lineas);
    return lineas_mejorado;
}

char** parsearPorSaltosDeLinea(char* buffer) {
    
    char** lineas = string_split(buffer, "\n");
    
    return lineas;
}

void inicializarListasGlobales(void ) { 


    listaNuevos =               list_create();
	listaReady =                list_create();
	listaBloqueados =           list_create();
	listaEjecutando =           list_create();
	listaFinalizados =          list_create();

    listaIO = list_create();
}

void iniciar_planificadores(){
        pthread_create(&planificadorCP, NULL, (void*) planificar_sig_to_running, (void*)SOCKET_CLIENTE);
        pthread_detach(planificadorCP);

}

void iniciarSemaforos(){
    pthread_mutex_init(&m_listaNuevos, NULL);
    pthread_mutex_init(&m_listaReady, NULL);
    sem_init(&proceso_en_ready,0, 0);

    }

void destruirSemaforos(){
	sem_destroy(&proceso_en_ready);

}

void planificar_sig_to_running(){ 

    while(1){
        sem_wait(&proceso_en_ready);

        if(!tieneDesalojo) { // FIFO
            pthread_mutex_lock(&m_listaReady);
            t_pcb* pcb_a_ejecutar = list_remove(listaReady, 0);
                pthread_mutex_unlock(&m_listaReady);

            log_info(kernel_logger, "agrego a RUNING y se lo paso a cpu para q ejecute!");

            cambiar_estado_a(pcb_a_ejecutar, RUNNING);
            agregar_a_lista_con_sems( pcb_a_ejecutar, listaEjecutando, listaEjecutando);

            paquete_pcb(cpu_dispatch_connection, pcb_a_ejecutar, EJECUTAR_PCB); // falta sacar comments a la conexion
            //eliminar_pcb(pcb_a_ejecutar)??
        }
        //logica hrrn?
        
    }

void enviar_Fin_consola(int socket){
    
    //pthread_mutex_lock(&mutexOk);
    
    t_paquete* paquete;
                        
    paquete = crear_paquete_op_code(FIN_CONSOLA);
            
    enviar_paquete(paquete, socket);
    
    eliminar_paquete(paquete);

    liberar_conexion(socket);
}

// recieve_handshake(socket_cliente);

    // t_list * lista;
    // while(1) {
    //     int cod_op = recibir_operacion(socket_cliente);
    //     log_trace(kernel_logger, "El codigo de operacion es %d",cod_op);

    //     switch (cod_op)
    //     {
    //     case MENSAJE:
    //         recibir_mensaje(socket_cliente,kernel_logger);
    //         break;
    //     case PAQUETE:
    //         lista = recibir_paquete(socket_cliente);
    //         log_trace(kernel_logger, "Paquete recibido");
    //         list_iterate(lista, (void*) iterator);
    //         break;

    //     case -1:
    //         log_error(kernel_logger, "El cliente se desconecto");
    //         return EXIT_FAILURE;
    //     default:
    //         log_warning(kernel_logger, "Operacion desconocida");
    //         return EXIT_FAILURE;
    //     }
    // }
    //recibir_mensaje(socket_cliente,kernel_logger);

/*
Logs minimos obligatorios
Creación de Proceso: Se crea el proceso <PID> en NEW
Fin de Proceso: Finaliza el proceso <PID> - Motivo: <SUCCESS / SEG_FAULT / OUT_OF_MEMORY>
Cambio de Estado: PID: <PID> - Estado Anterior: <ESTADO_ANTERIOR> - Estado Actual: <ESTADO_ACTUAL>
Motivo de Bloqueo: PID: <PID> - Bloqueado por: <IO / NOMBRE_RECURSO / NOMBRE_ARCHIVO>
I/O:  PID: <PID> - Ejecuta IO: <TIEMPO>
Ingreso a Ready: Cola Ready <ALGORITMO>: [<LISTA DE PIDS>]
Wait: PID: <PID> - Wait: <NOMBRE RECURSO> - Instancias: <INSTANCIAS RECURSO>    Nota: El valor de las instancias es después de ejecutar el Wait
Signal: PID: <PID> - Signal: <NOMBRE RECURSO> - Instancias: <INSTANCIAS RECURSO>    Nota: El valor de las instancias es después de ejecutar el Signal
Crear Segmento: PID: <PID> - Crear Segmento - Id: <ID SEGMENTO> - Tamaño: <TAMAÑO>
Eliminar Segmento: PID: <PID> - Eliminar Segmento - Id Segmento: <ID SEGMENTO>
Inicio Compactación: Compactación: <Se solicitó compactación / Esperando Fin de Operaciones de FS>
Fin Compactación: Se finalizó el proceso de compactación
Abrir Archivo: PID: <PID> - Abrir Archivo: <NOMBRE ARCHIVO>
Cerrar Archivo: PID: <PID> - Cerrar Archivo: <NOMBRE ARCHIVO>
Actualizar Puntero Archivo: PID: <PID> - Actualizar puntero Archivo: <NOMBRE ARCHIVO> - Puntero <PUNTERO>   Nota: El valor del puntero debe ser luego de ejecutar F_SEEK.
Truncar Archivo: PID: <PID> - Archivo: <NOMBRE ARCHIVO> - Tamaño: <TAMAÑO>
Leer Archivo: PID: <PID> - Leer Archivo: <NOMBRE ARCHIVO> - Puntero <PUNTERO> - Dirección Memoria <DIRECCIÓN MEMORIA> - Tamaño <TAMAÑO>
Escribir Archivo: PID: <PID> -  Escribir Archivo: <NOMBRE ARCHIVO> - Puntero <PUNTERO> - Dirección Memoria <DIRECCIÓN MEMORIA> - Tamaño <TAMAÑO>


Orden de inicio de servidores: Memoria -> FileSystem/CPU (Ambos tienen que estar levantados) -> Kernel -> Consola

Consola en multiples instancias va a: levantar su configuracion inicial -> conectarse con el servidor de kernel -> hacer un handshake -> mandar el paquete con el pseudocodigo ->
recibir un mensaje de llegada de pseudocodigo -> esperar a otro mensaje como SUCCES - SEG_FAULT - OUT_OF_MEMORY -> finaliza el programa de consola

kernel va a: levantar su configuracion inicial -> levantar el servidor -> conectarse con los servidores de CPU - Memoria - FileSystem ->
recibir Multiplexando clientes consola -> hacer el hanshake -> recibir el archivo de pseudocodigo y enviar mensaje de buena llegada -> ejecutar el proceso ->
enviar a consola el mensaje de finalizacion ya sea SUCCES - SEG_FAULT - OUT_OF_MEMORY

Checkpoint 2
Levanta el archivo de configuración - DONE
Se conecta a CPU, Memoria y File System - ALMOST DONE
Espera conexiones de las consolas - DONE
Recibe de las consolas las instrucciones y arma el PCB - ALMOST ALMOST DONE
Planificación de procesos con FIFO - REMAINS

*/