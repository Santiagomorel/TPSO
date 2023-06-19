#include "kernel.h"

int main(int argc, char **argv)
{
    // ----------------------- creo el log del kernel ----------------------- //

    kernel_logger = init_logger("./runlogs/kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);

    // ----------------------- levanto y cargo la configuracion del kernel ----------------------- //
    log_info(kernel_logger, "INICIA EL MODULO DE KERNEL");

    log_trace(kernel_logger, "levanto la configuracion del kernel");
    if (argc < 2)
    {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }

    kernel_config_file = init_config(argv[1]);

    if (kernel_config_file == NULL)
    {
        perror("Ocurrió un error al intentar abrir el archivo config");
        exit(1);
    }

    log_trace(kernel_logger, "cargo la configuracion del kernel");

    load_config();

    inicializarListasGlobales();

    iniciarSemaforos();

    // ----------------------- contecto el kernel con los servidores de MEMORIA - CPU (dispatch) - FILESYSTEM ----------------------- //
    iniciar_conexiones_kernel();
    
    // ----------------------- levanto el servidor del kernel ----------------------- //

    socket_servidor_kernel = iniciar_servidor(kernel_config.puerto_escucha, kernel_logger);
    log_trace(kernel_logger, "kernel inicia el servidor");

    // ----------------------- inicio los planificadores (no cambiar orden) ----------------------- //

    iniciar_planificadores();

    // ----------------------- espero conexiones de consola ----------------------- //

    while (1)
    {

        log_trace(kernel_logger, "esperando cliente consola");
        socket_cliente = esperar_cliente(socket_servidor_kernel, kernel_logger);
        log_trace(kernel_logger, "entro una consola con el socket: %d", socket_cliente);

        pthread_t atiende_consola;
        pthread_create(&atiende_consola, NULL, (void *)recibir_consola, (void *) (intptr_t) socket_cliente);
        pthread_detach(atiende_consola);
    }

    return EXIT_SUCCESS;
}

// ----------------------- Funciones de inicio de Kernel ----------------------- //

void load_config()
{

    kernel_config.ip_memoria = config_get_string_value(kernel_config_file, "IP_MEMORIA");
    kernel_config.puerto_memoria = config_get_string_value(kernel_config_file, "PUERTO_MEMORIA");
    kernel_config.ip_file_system = config_get_string_value(kernel_config_file, "IP_FILESYSTEM");
    kernel_config.puerto_file_system = config_get_string_value(kernel_config_file, "PUERTO_FILESYSTEM");
    kernel_config.ip_cpu = config_get_string_value(kernel_config_file, "IP_CPU");
    kernel_config.puerto_cpu = config_get_string_value(kernel_config_file, "PUERTO_CPU");
    kernel_config.puerto_escucha = config_get_string_value(kernel_config_file, "PUERTO_ESCUCHA");
    kernel_config.algoritmo_planificacion = config_get_string_value(kernel_config_file, "ALGORITMO_PLANIFICACION");

    kernel_config.estimacion_inicial = config_get_int_value(kernel_config_file, "ESTIMACION_INICIAL");

    kernel_config.hrrn_alfa = config_get_long_value(kernel_config_file, "HRRN_ALFA");

    kernel_config.grado_max_multiprogramacion = config_get_int_value(kernel_config_file, "GRADO_MAX_MULTIPROGRAMACION");

    kernel_config.recursos = config_get_array_value(kernel_config_file, "RECURSOS");
    kernel_config.instancias_recursos = convertirPunteroCaracterAEntero(config_get_array_value(kernel_config_file, "INSTANCIAS_RECURSOS"));

    log_trace(kernel_logger, "config cargada en 'kernel_cofig_file'");
}

int* convertirPunteroCaracterAEntero(char** punteroCaracteres)
{
    int size = string_array_size(punteroCaracteres);
    cantidad_instancias = size;
    int* intArray = (int*)malloc(size * sizeof(int)); // hacer free del intArray

        for (int i = 0; i < size; ++i) {
            intArray[i] = atoi(punteroCaracteres[i]);
        }
        
    return intArray;
}

void inicializarListasGlobales()
{
    listaNuevos = list_create();
    listaReady = list_create();
    listaBloqueados = list_create();
    listaEjecutando = list_create();
    listaFinalizados = list_create();
    iniciar_listas_recursos(kernel_config.recursos);

    listaIO = list_create();
}

void iniciar_listas_recursos(char** recursos)
{
    int cantidad = string_array_size(recursos);
    if (cantidad > MAX_RECURSOS) {
        log_error(kernel_logger, "La cantidad de recursos en config excede el limite establecido por el programa");
        exit(1);
    }
    
    lista_recurso = (t_list**)malloc(cantidad_instancias * sizeof(t_list*));

    for (int i = 0; i < cantidad; i++) {
        lista_recurso[i] = list_create();
    }

    log_info(kernel_logger, "Se inician las listas de los recursos");
}

void iniciarSemaforos()
{
    pthread_mutex_init(&m_listaNuevos, NULL);
    pthread_mutex_init(&m_listaReady, NULL);
    pthread_mutex_init(&m_listaFinalizados, NULL);
    pthread_mutex_init(&m_listaBloqueados, NULL);
    pthread_mutex_init(&m_listaEjecutando, NULL);
    pthread_mutex_init(&m_contador_id, NULL);
    sem_init(&proceso_en_ready, 0, 0);
    sem_init(&fin_ejecucion, 0, 1);
    sem_init(&grado_multiprog, 0, kernel_config.grado_max_multiprogramacion);
    iniciar_semaforos_recursos(kernel_config.recursos, kernel_config.instancias_recursos);
}

void iniciar_semaforos_recursos(char** recursos, int* instancias_recursos) // VER SI ES NECESARIO UTILIZARLOS
{
    int cantidad_recursos = string_array_size(recursos);

    if (cantidad_instancias != cantidad_recursos) {
        log_error(kernel_logger, "La cantidad de instancias de recursos es diferente a la cantidad de recursos en config");
        exit(1);
    }

    for (int i = 0; i < cantidad_recursos; i++) {
        m_listaRecurso[i] = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)); // VER luego hay que eliminar los malloc
        pthread_mutex_init(m_listaRecurso[i], NULL); //sem_wait(sem_recurso[x])
    }

    for (int i = 0; i < cantidad_recursos; i++) {
        sem_recurso[i] = (sem_t*)malloc(sizeof(sem_t)); // VER luego hay que eliminar los malloc
        sem_init(sem_recurso[i], 0, instancias_recursos[i]); //sem_wait(sem_recurso[x])
    }
    log_info(kernel_logger, "Se inician los semaforos de los recursos");
}

void iniciar_conexiones_kernel()
{
    if((memory_connection = crear_conexion(kernel_config.ip_memoria , kernel_config.puerto_memoria)) == -1) {
        log_error(kernel_logger, "No se pudo conectar al servidor de MEMORIA");
        exit(2);
    }
    recibir_operacion(memory_connection);
    recibir_mensaje(memory_connection, kernel_logger);

    if((cpu_dispatch_connection = crear_conexion(kernel_config.ip_cpu , kernel_config.puerto_cpu)) == -1) {
        log_error(kernel_logger, "No se pudo conectar al servidor de CPU DISPATCH");
        exit(2);
    }
    recibir_operacion(cpu_dispatch_connection);
    recibir_mensaje(cpu_dispatch_connection, kernel_logger);

    if((file_system_connection = crear_conexion(kernel_config.ip_file_system , kernel_config.puerto_file_system)) == -1) {
        log_error(kernel_logger, "No se pudo conectar al servidor de FILE SYSTEM");
        exit(2);
    }
    recibir_operacion(file_system_connection);
    recibir_mensaje(file_system_connection, kernel_logger);
}

void iniciar_planificadores()
{
    pthread_create(&planificadorCP, NULL, (void*)planificar_sig_to_running, (void*) (intptr_t)socket_cliente);
    pthread_detach(planificadorCP);

    pthread_create(&hiloDispatch, NULL, (void*)manejar_dispatch, (void*) (intptr_t)cpu_dispatch_connection);
    pthread_detach(hiloDispatch);
}

// ----------------------- Funciones relacionadas con consola ----------------------- //

void recibir_consola(int SOCKET_CLIENTE)
{
    int codigoOperacion = recibir_operacion(SOCKET_CLIENTE);

    switch (codigoOperacion)
    {
    case MENSAJE:
        log_trace(kernel_logger, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);

        break;
    // ---------LP entrante----------
    case INICIAR_PCB:

        log_trace(kernel_logger, "consola envia pseudocodigo, inicio PCB");
        t_pcb *pcb_a_iniciar = iniciar_pcb(SOCKET_CLIENTE);
        log_trace(kernel_logger, "pcb iniciado PID : %d", pcb_a_iniciar->id);
        pthread_mutex_lock(&m_listaNuevos);
        list_add(listaNuevos, pcb_a_iniciar);
        pthread_mutex_unlock(&m_listaNuevos);
        log_info(kernel_logger, "Se crea el proceso [%d] en [NEW]", pcb_a_iniciar->id);

        enviar_mensaje("Llego el paquete", SOCKET_CLIENTE);

        planificar_sig_to_ready(); // usar esta funcion cada vez q se agregue un proceso a NEW o BLOCKED
        break;

    default:
        log_trace(kernel_logger, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
        break;
    }
}

t_pcb* iniciar_pcb(int socket)
{
    int size = 0;

    char *buffer;
    int desp = 0;

    buffer = recibir_buffer(&size, socket);

    char *instrucciones = leer_string(buffer, &desp);

    t_pcb *nuevo_pcb = pcb_create(instrucciones, socket);
    free(instrucciones);
    free(buffer);

    loggear_pcb(nuevo_pcb, kernel_logger);

    return nuevo_pcb;
}

t_pcb* pcb_create(char* instrucciones, int socket_consola)
{
    log_trace(kernel_logger, "instrucciones en pcb create %s", instrucciones);
    t_pcb* new_pcb = malloc(sizeof(t_pcb));

    generar_id(new_pcb);
    new_pcb->instrucciones = separar_inst_en_lineas(instrucciones);
    new_pcb->program_counter = 0;
    new_pcb->registros_cpu = crear_registros();
    new_pcb->tabla_segmentos = list_create();
    new_pcb->estimacion_rafaga = kernel_config.estimacion_inicial;
    new_pcb->tiempo_llegada_ready = temporal_create();
    // new_pcb->tabla_archivos_abiertos = ; // TODO tabla de archivos abiertos
    
    new_pcb->salida_ejecucion = temporal_create();
    new_pcb->rafaga_ejecutada = 0;
    
    new_pcb->calculoRR = 1;
    new_pcb->socket_consola = socket_consola;

    new_pcb->estado_actual = NEW;
    new_pcb->socket_consola = socket_consola;

    return new_pcb;
}

void generar_id(t_pcb* pcb)
{ // Con esta funcion le asignamos a un pcb su id, cuidando de no repetir el numero
    pthread_mutex_lock(&m_contador_id);
    pcb->id = contador_id;
    contador_id++;
    pthread_mutex_unlock(&m_contador_id);
}

char** separar_inst_en_lineas(char* instrucciones)
{
    char **lineas = parsearPorSaltosDeLinea(instrucciones);

    char **lineas_mejorado = string_array_new();
    int i;

    for (i = 0; i < string_array_size(lineas); i++)
    {
        char **instruccion = string_split(lineas[i], " "); // separo por espacios
        //log_trace(kernel_logger, "Agrego la linea '%s' a las intrucciones del pcb", lineas[i]);
        string_array_push(&lineas_mejorado, string_duplicate(lineas[i]));
    }
    string_array_destroy(lineas);
    return lineas_mejorado;
}

char** parsearPorSaltosDeLinea(char* buffer)
{
    char **lineas = string_split(buffer, "\n");

    return lineas;
}

t_registro* crear_registros()
{
    t_registro * nuevosRegistros = malloc(sizeof(t_registro));
    strncpy(nuevosRegistros->AX , "HOLA", 4);
    strncpy(nuevosRegistros->BX , "CHAU", 4);
    strncpy(nuevosRegistros->CX , "TEST", 4);
    strncpy(nuevosRegistros->DX , "ABCD", 4);
	strncpy(nuevosRegistros->EAX , "PABLITOS", 8);
	strncpy(nuevosRegistros->EBX , "HERMANOS", 8);
	strncpy(nuevosRegistros->ECX , "12345678", 8);
	strncpy(nuevosRegistros->EDX , "87654321", 8);
	strncpy(nuevosRegistros->RAX , "PLISSTOPIMINPAIN", 16);
	strncpy(nuevosRegistros->RBX , "PLISSTOPIMINPAIN", 16);
	strncpy(nuevosRegistros->RCX , "PLISSTOPIMINPAIN", 16);
	strncpy(nuevosRegistros->RDX , "PLISSTOPIMINPAIN", 16);
    return nuevosRegistros;
}

// ----------------------- Funciones relacionadas con planificadores ----------------------- //

void cambiar_estado_a(t_pcb* a_pcb, estados nuevo_estado, estados estado_anterior)
{
    a_pcb->estado_actual = nuevo_estado;
    log_info(kernel_logger,"Cambio de Estado: PID: [%d] - Estado Anterior: [%s] - Estado Actual: [%s]", obtenerPid(a_pcb), obtenerEstado(estado_anterior), obtenerEstado(nuevo_estado));
}

int obtenerPid(t_pcb* pcb)
{
    return pcb->id;
}

char* obtenerEstado(estados estado)
{
    switch(estado)
    {
        case NEW:
        return "NEW";
        break;
        case READY:
        return "READY";
        break;
        case BLOCKED:
        return "BLOCKED";
        break;
        case RUNNING:
        return "RUNNING";
        break;
        case EXIT:
        return "EXIT";
        break;
        default:
        return "Error de estado";
    }
}

int estadoActual(t_pcb* pcb) //VER
{
    return pcb->estado_actual;
}

void agregar_a_lista_con_sems(t_pcb* pcb_a_agregar, t_list* lista, pthread_mutex_t m_sem)
{
    pthread_mutex_lock(&m_sem);
    agregar_lista_ready_con_log(lista,pcb_a_agregar,kernel_config.algoritmo_planificacion);
    pthread_mutex_unlock(&m_sem);
}

// ----------------------- Funciones planificador to - ready ----------------------- //

void planificar_sig_to_ready()
{
    sem_wait(&grado_multiprog);

    // if (!list_is_empty(listaBloqueados) && list_any_satisfy(listaBloqueados, bloqueado_termino_io))
    // { // BLOCKED -> READY

    //     pthread_mutex_lock(&m_listaBloqueados);
    //     t_pcb *pcb_a_ready = list_remove_by_condition(listaBloqueados, bloqueado_termino_io);
    //     pthread_mutex_unlock(&m_listaBloqueados);
    //     // PRESCENCIA=1
    //     // pcb_a_ready->tabla_paginas = pedir_tabla_pags(pcb_a_ready, conexion_memoria); // TODO pedir_tabla_pags
        
    //     cambiar_estado_a(pcb_a_ready, READY, estadoActual(pcb_a_ready));
    //     agregar_a_lista_con_sems(pcb_a_ready, listaReady, m_listaReady);

    //     sem_post(&proceso_en_ready);
    // }
    if (!list_is_empty(listaNuevos)) // NEW -> READY
    {
        pthread_mutex_lock(&m_listaNuevos);
        t_pcb *pcb_a_ready = list_remove(listaNuevos, 0);
        pthread_mutex_unlock(&m_listaNuevos);

        log_trace(kernel_logger, "Inicializamos estructuras del pcb en memoria");
        inicializar_estructuras(pcb_a_ready);

        /*t_list *nuevo_segmento =*/ pedir_tabla_segmentos();
        // t_segmento * primerSegmentoNuevo = list_get(nuevo_segmento,0);
        // log_error(kernel_logger, "info de la tabla de segmentos pedida: %d, %d, %d", primerSegmentoNuevo->tamanio_segmento,primerSegmentoNuevo->id_segmento, primerSegmentoNuevo->direccion_base);

        // list_add_all(pcb_a_ready->tabla_segmentos, nuevo_segmento);

        // t_segmento * posibleSegmento = list_get(pcb_a_ready->tabla_segmentos, 0);
        // log_error(kernel_logger, "info (tamanio de segmento) de la tabla de segmentos creada: %d, %d, %d", posibleSegmento->tamanio_segmento,posibleSegmento->id_segmento, posibleSegmento->direccion_base);
        cambiar_estado_a(pcb_a_ready, READY, estadoActual(pcb_a_ready));
        agregar_a_lista_con_sems(pcb_a_ready, listaReady, m_listaReady);
        sem_post(&proceso_en_ready);
    }
}

void inicializar_estructuras(t_pcb* pcb)
{
    t_paquete *paquete = crear_paquete_op_code(INICIAR_ESTRUCTURAS);
    agregar_entero_a_paquete(paquete, pcb->id);

    enviar_paquete(paquete, memory_connection);

    eliminar_paquete(paquete);
}

void pedir_tabla_segmentos() // MODIFICAR tipo de dato que devuelve
{
    int codigoOperacion = recibir_operacion(memory_connection);
    if (codigoOperacion != TABLA_SEGMENTOS) //TABLA_SEGMENTOS MODIFICAR cuando tengamos la tabla de segmentos
    {
        log_trace(kernel_logger, "llego otra cosa q no era un tabla pags :c");
    }
    
    recibir_mensaje(memory_connection, kernel_logger);
    //return recibir_paquete(memory_connection);
}

// ----------------------- Funciones planificador to - running ----------------------- //

void planificar_sig_to_running()
{
    while(1){
        sem_wait(&fin_ejecucion);
        sem_wait(&proceso_en_ready);
        log_trace(kernel_logger, "Entra en la planificacion de READY RUNNING");
        if(strcmp(kernel_config.algoritmo_planificacion, "FIFO") == 0) { // FIFO
            //log_info(kernel_logger, "Cola Ready FIFO: %s", funcionQueMuestraPID()); // HACER // VER LOCALIZACION
            pthread_mutex_lock(&m_listaReady);
            t_pcb* pcb_a_ejecutar = list_remove(listaReady, 0);
            pthread_mutex_unlock(&m_listaReady);

            funcion_agregar_running(pcb_a_ejecutar);
        }
        else if (strcmp(kernel_config.algoritmo_planificacion, "HRRN") == 0){ // HRRN
            int tamanioLista = list_size(listaReady);
            if(tamanioLista == 1){
                log_trace(kernel_logger, "El tamanio de la lista de ready es 1, hago FIFO");
                pthread_mutex_lock(&m_listaReady);
                t_pcb* pcb_a_ejecutar = list_remove(listaReady, 0);
                pthread_mutex_unlock(&m_listaReady);
                funcion_agregar_running(pcb_a_ejecutar);
            }else{
                log_trace(kernel_logger, "El tamanio de la lista de ready es MAYOR");
                t_pcb* pcb_a_ejecutar = list_get_maximum(listaReady,(void*) mayorRRdeLista);
                setear_estimacion(pcb_a_ejecutar);
                pthread_mutex_lock(&m_listaReady);
                list_remove_element(listaReady, pcb_a_ejecutar);
                pthread_mutex_unlock(&m_listaReady);
                funcion_agregar_running(pcb_a_ejecutar);
            }
              //log_info(kernel_logger,"HRRN: Hay %d procesos listos para ejecutar",list_size(listaReady);
        }
    }
}

void funcion_agregar_running(t_pcb* pcb_a_ejecutar)
{
    iniciar_tiempo_ejecucion(pcb_a_ejecutar);
    cambiar_estado_a(pcb_a_ejecutar, RUNNING, estadoActual(pcb_a_ejecutar));
    agregar_a_lista_con_sems(pcb_a_ejecutar, listaEjecutando, m_listaEjecutando);
    contexto_ejecucion * nuevoContexto = obtener_ce(pcb_a_ejecutar);
    enviar_ce(cpu_dispatch_connection, nuevoContexto, EJECUTAR_CE, kernel_logger);
    log_trace(kernel_logger, "Agrego un proceso a running y envio el contexto de ejecucion");
}

void iniciar_tiempo_ejecucion(t_pcb* pcb)
{
    temporal_destroy(pcb->salida_ejecucion);
    pcb->salida_ejecucion = temporal_create();
}

void setear_estimacion(t_pcb* pcb)
{
    if(pcb->rafaga_ejecutada){
        pcb->estimacion_rafaga = ((pcb->calculoRR)*1000);
        pcb->calculoRR = 0;
    }
}

// ----------------------- Funciones calculo HRRN ----------------------- //

t_pcb* mayorRRdeLista(void* _pcb1, void* _pcb2)
{
        t_pcb* pcb1 = (t_pcb*)_pcb1;

        t_pcb* pcb2 = (t_pcb*)_pcb2;

        return mayorRR(pcb1,pcb2);
}

t_pcb* mayorRR(t_pcb* pcb1, t_pcb* pcb2)
{
    double RR_pcb1 = calcularRR(pcb1);

    double RR_pcb2 = calcularRR(pcb2);

    //log_info(logger,"Comparo pcb1 [%d] y pcb2[%d], RR_pcb1 [%d] y RR_pcb2 [%d] ",pcb1->pid, pcb2->pid,RR_pcb1,RR_pcb2 );

    if (RR_pcb1 == RR_pcb2){
        return mayor_prioridad_PID(pcb1,pcb2);
    }
    else if (RR_pcb1 > RR_pcb2) {
        return pcb1;
    }
    else {
        return pcb2;
    }
}

double calcularRR(t_pcb* pcb)
{
    double tiempoEspera = (temporal_gettime(pcb->tiempo_llegada_ready)/1000);
    if (pcb->rafaga_ejecutada) {
        log_trace(kernel_logger, "El proceso PID %d calcula RR CON CALCULO, rafaga e. de %ld ms", pcb->id, pcb->rafaga_ejecutada);
        double calculo = calculoEstimado(pcb);
        double valorRetorno = round(1 + (tiempoEspera/(calculo/1000)));
        pcb->calculoRR = valorRetorno;
        return valorRetorno;
    } else {
        log_trace(kernel_logger, "El proceso PID %d calcula RR ESTIMACION INICIAL", pcb->id);
        double valorRetorno =  (1 + (tiempoEspera/(pcb->estimacion_rafaga/1000)));
        return valorRetorno;
    }
}

double calculoEstimado(t_pcb* pcb)
{
    double alfa = kernel_config.hrrn_alfa;

    return ((alfa * pcb->estimacion_rafaga) + ( (1 - alfa) * pcb->rafaga_ejecutada));
}

t_pcb* mayor_prioridad_PID(t_pcb* pcb1, t_pcb* pcb2)
{
    if(pcb1->id < pcb2->id){
        return pcb1;
    } else {
        return pcb2;
    }
}

// ----------------------- Funciones planificador blocked ----------------------- //


// ----------------------- Funciones CE ----------------------- //

contexto_ejecucion* obtener_ce(t_pcb* pcb)
{ // PENSAR EN HACERLO EN   AMBOS SENTIDOS
    contexto_ejecucion * nuevoContexto = malloc(sizeof(contexto_ejecucion));
    nuevoContexto->instrucciones = string_array_new();
    nuevoContexto->registros_cpu = malloc(sizeof(t_registro));
    log_trace(kernel_logger, "hace maloc del nuevo contexto");
    copiar_id_pcb_a_ce(pcb, nuevoContexto);
    log_trace(kernel_logger, "copia el id del pcb");
    copiar_instrucciones_pcb_a_ce(pcb, nuevoContexto);
    log_trace(kernel_logger, "copia instrucciones del pcb");
    copiar_PC_pcb_a_ce(pcb, nuevoContexto);
    log_trace(kernel_logger, "copia program counter");
    copiar_registros_pcb_a_ce(pcb, nuevoContexto);
    // copiar_tabla_segmentos(pcb, nuevoContexto);    // FALTA HACER
    return nuevoContexto;
}

void copiar_id_pcb_a_ce(t_pcb* pcb, contexto_ejecucion* ce)
{
    ce->id = pcb->id;
}

void copiar_instrucciones_pcb_a_ce(t_pcb* pcb, contexto_ejecucion* ce)
{
    for (int i = 0; i < string_array_size(pcb->instrucciones); i++) {
        // log_trace(kernel_logger, "copio en ce %s", pcb->instrucciones[i]);
        string_array_push(&(ce->instrucciones), string_duplicate(pcb->instrucciones[i]));
    }
}

void copiar_PC_pcb_a_ce(t_pcb* pcb, contexto_ejecucion* ce)
{
    ce->program_counter = pcb->program_counter;
}

void copiar_PC_ce_a_pcb(contexto_ejecucion* ce, t_pcb* pcb)
{
    pcb->program_counter = ce->program_counter;
}

void copiar_registros_pcb_a_ce(t_pcb* pcb, contexto_ejecucion* ce)
{
    strcpy(ce->registros_cpu->AX , pcb->registros_cpu->AX);
    strcpy(ce->registros_cpu->BX , pcb->registros_cpu->BX);
    strcpy(ce->registros_cpu->CX , pcb->registros_cpu->CX);
    strcpy(ce->registros_cpu->DX , pcb->registros_cpu->DX);
	strcpy(ce->registros_cpu->EAX , pcb->registros_cpu->EAX);
	strcpy(ce->registros_cpu->EBX , pcb->registros_cpu->EBX);
	strcpy(ce->registros_cpu->ECX , pcb->registros_cpu->ECX);
	strcpy(ce->registros_cpu->EDX , pcb->registros_cpu->EDX);
	strcpy(ce->registros_cpu->RAX , pcb->registros_cpu->RAX);
	strcpy(ce->registros_cpu->RBX , pcb->registros_cpu->RBX);
	strcpy(ce->registros_cpu->RCX , pcb->registros_cpu->RCX);
	strcpy(ce->registros_cpu->RDX , pcb->registros_cpu->RDX);
}

void copiar_registros_ce_a_pcb(contexto_ejecucion* ce, t_pcb* pcb)
{
    strcpy(pcb->registros_cpu->AX , ce->registros_cpu->AX);
    strcpy(pcb->registros_cpu->BX , ce->registros_cpu->BX);
    strcpy(pcb->registros_cpu->CX , ce->registros_cpu->CX);
    strcpy(pcb->registros_cpu->DX , ce->registros_cpu->DX);
	strcpy(pcb->registros_cpu->EAX , ce->registros_cpu->EAX);
	strcpy(pcb->registros_cpu->EBX , ce->registros_cpu->EBX);
	strcpy(pcb->registros_cpu->ECX , ce->registros_cpu->ECX);
	strcpy(pcb->registros_cpu->EDX , ce->registros_cpu->EDX);
	strcpy(pcb->registros_cpu->RAX , ce->registros_cpu->RAX);
	strcpy(pcb->registros_cpu->RBX , ce->registros_cpu->RBX);
	strcpy(pcb->registros_cpu->RCX , ce->registros_cpu->RCX);
	strcpy(pcb->registros_cpu->RDX , ce->registros_cpu->RDX);
}

// ----------------------- Funciones Dispatch Manager ----------------------- //
void manejar_dispatch()
{
    log_trace(kernel_logger, "Entre por manejar dispatch");
    while(1){
        int cod_op = recibir_operacion(cpu_dispatch_connection);
        switch(cod_op){
            case SUCCESS:
            case EXIT_ERROR_RECURSO:
            case SEG_FAULT:
                contexto_ejecucion* contexto_a_finalizar = recibir_ce(cpu_dispatch_connection);
                pthread_mutex_lock(&m_listaEjecutando);
                    t_pcb * pcb_a_finalizar = (t_pcb *) list_remove(listaEjecutando, 0); // inicializar pcb y despues liberarlo
                    actualizar_pcb(pcb_a_finalizar, contexto_a_finalizar); //FALTA HACER VER
                pthread_mutex_unlock(&m_listaEjecutando);
                cambiar_estado_a(pcb_a_finalizar, EXIT, estadoActual(pcb_a_finalizar));

                //sem_post(&cpu_libre_para_ejecutar); // si es FIFO no se usa, esto no deberia provocar nada, revisar eso iguañ
                sem_post(&grado_multiprog); // NUEVO grado_multiprog
                sem_post(&fin_ejecucion);
                //signal(liberar);
                //paquete_fin_memoria(pcb_a_finalizar->id, pcb_a_finalizar->tabla_paginas);
                //wait (liberado);
                pthread_mutex_lock(&m_listaFinalizados);
                    list_add(listaFinalizados, pcb_a_finalizar);
                pthread_mutex_unlock(&m_listaFinalizados);
                
                log_info(kernel_logger, "Finaliza el proceso [%d] - Motivo: [%s]", pcb_a_finalizar->id, obtenerCodOP(cod_op));
                enviar_Fin_consola(pcb_a_finalizar->socket_consola); 
                liberar_ce(contexto_a_finalizar);
                //eliminar(pcb_a_finalizar);

                break;
            case DESALOJO_YIELD:
                contexto_ejecucion* contexto_a_reencolar = recibir_ce(cpu_dispatch_connection);
                pthread_mutex_lock(&m_listaEjecutando);
                    t_pcb * pcb_a_reencolar = (t_pcb *) list_remove(listaEjecutando, 0); // inicializar pcb y despues liberarlo
                    actualizar_pcb(pcb_a_reencolar, contexto_a_reencolar);
                pthread_mutex_unlock(&m_listaEjecutando);
                cambiar_estado_a(pcb_a_reencolar, READY, estadoActual(pcb_a_reencolar));
                sacar_rafaga_ejecutada(pcb_a_reencolar); // hacer cada vez que sale de running
                iniciar_nueva_espera_ready(pcb_a_reencolar); // hacer cada vez que se mete en la lista de ready
                pthread_mutex_lock(&m_listaReady);
                agregar_lista_ready_con_log(listaReady, pcb_a_reencolar,kernel_config.algoritmo_planificacion);
                pthread_mutex_unlock(&m_listaReady);
                sem_post(&proceso_en_ready);
                sem_post(&fin_ejecucion);

                liberar_ce(contexto_a_reencolar);
                //eliminar(pcb_a_reencolar);
                break;
            case WAIT_RECURSO:
                char* recurso_wait = recibir_string(cpu_dispatch_connection, kernel_logger);
                log_warning(kernel_logger, "recibo el recurso_wait como %s", recurso_wait);

                if(recurso_no_existe(recurso_wait)){
                    enviar_CodOp(cpu_dispatch_connection, NO_EXISTE_RECURSO);
                }
                else {
                    int id_recurso = obtener_id_recurso(recurso_wait);
                    restar_instancia(id_recurso);
                    if(tiene_instancia_wait(id_recurso)){
                        enviar_CodOp(cpu_dispatch_connection, LO_TENGO);
                        sem_wait(sem_recurso[id_recurso]);
                        // recibir_operacion(cpu_dispatch_connection);
                        // contexto_ejecucion* contexto_ejecuta_wait = recibir_ce(cpu_dispatch_connection);
                        // pthread_mutex_lock(&m_listaEjecutando);
                        //     t_pcb * pcb_wait = (t_pcb *) list_get(listaEjecutando, 0);
                        //     actualizar_pcb(pcb_wait, contexto_ejecuta_wait);
                        // pthread_mutex_unlock(&m_listaEjecutando);
                        // log_trace(kernel_logger,"PID: %d-- Wait: %s, - Instancias:%d",,recurso_wait,);//aca necesito el PID y las instancias no las
                    } else {
                        enviar_CodOp(cpu_dispatch_connection, NO_LO_TENGO);
                        recibir_operacion(cpu_dispatch_connection);
                        contexto_ejecucion* contexto_bloqueado_en_recurso= recibir_ce(cpu_dispatch_connection);
                        pthread_mutex_lock(&m_listaEjecutando);
                            t_pcb * pcb_bloqueado_en_recurso = (t_pcb *) list_remove(listaEjecutando, 0); // inicializar pcb y despues liberarlo
                            actualizar_pcb(pcb_bloqueado_en_recurso, contexto_bloqueado_en_recurso);
                        pthread_mutex_unlock(&m_listaEjecutando);
                        cambiar_estado_a(pcb_bloqueado_en_recurso, BLOCKED, estadoActual(pcb_bloqueado_en_recurso));
                        log_info(kernel_logger, "PID: [%d] - Bloqueado por: [%s]",pcb_bloqueado_en_recurso->id, recurso_wait);
                        sacar_rafaga_ejecutada(pcb_bloqueado_en_recurso); // hacer cada vez que sale de running
                        sem_post(&fin_ejecucion);
                        liberar_ce(contexto_bloqueado_en_recurso);
                        bloqueo_proceso_en_recurso(pcb_bloqueado_en_recurso, id_recurso); // aca tengo que encolar en la lista correspondiente
                    }
                }
                break; 
            case SIGNAL_RECURSO:
                // contexto_ejecucion* contexto_ejecuta_signal = recibir_ce(cpu_dispatch_connection);
                // recibir_operacion(cpu_dispatch_connection);
                char* recurso_signal = recibir_string(cpu_dispatch_connection, kernel_logger);
                if(recurso_no_existe(recurso_signal)){
                    enviar_CodOp(cpu_dispatch_connection, NO_EXISTE_RECURSO);
                }else {
                    int id_recurso = obtener_id_recurso(recurso_wait);
                    enviar_CodOp(cpu_dispatch_connection, LO_TENGO);
                    // int id_recurso = obtener_id_recurso(recurso_signal);
                    sumar_instancia(id_recurso);
                    // pthread_mutex_lock(&m_listaEjecutando);
                    // t_pcb * pcb_signal = (t_pcb *) list_get(listaEjecutando, 0);
                    // actualizar_pcb(pcb_signal, contexto_ejecuta_signal);
                    // pthread_mutex_unlock(&m_listaEjecutando);
                    sem_post(sem_recurso[id_recurso]);
                    if(tiene_que_reencolar_bloq_recurso(id_recurso)){
                        reencolar_bloqueo_por_recurso(id_recurso);
                    }
                // log_trace(kernel_logger,"PID: %d-- Signal: %s, - Instancias:%d",contexto_IO->id,recurso_wait,);//aca necesito el PID y las instancias no las tengo

                }
                break;
            //case BLOCK_por_PF:
            case -1:
                break;
            



            case BLOCK_IO:
                // log_trace(kernel_logger,"recibi io");
                char* tiempo_bloqueo = recibir_string(cpu_dispatch_connection, kernel_logger);
                recibir_operacion(cpu_dispatch_connection);
                contexto_ejecucion* contexto_IO = recibir_ce(cpu_dispatch_connection);
                int bloqueo = atoi(tiempo_bloqueo);
                pthread_mutex_lock(&m_listaEjecutando);
                    t_pcb * pcb_IO = (t_pcb *) list_remove(listaEjecutando, 0); // inicializar pcb y despues liberarlo
                    actualizar_pcb(pcb_IO, contexto_IO);
                pthread_mutex_unlock(&m_listaEjecutando);
                cambiar_estado_a(pcb_IO, BLOCKED, estadoActual(pcb_IO));
                sacar_rafaga_ejecutada(pcb_IO); // hacer cada vez que sale de running
                sem_post(&fin_ejecucion);
                log_info(kernel_logger, "PID: [%d] - Bloqueado por: [IO]",pcb_IO->id);
                log_info(kernel_logger,"PID: [%d] - Ejecuta IO: [%d]",pcb_IO->id, bloqueo);
                sleep(bloqueo);
                cambiar_estado_a(pcb_IO, READY, estadoActual(pcb_IO));
                iniciar_nueva_espera_ready(pcb_IO); // hacer cada vez que se mete en la lista de ready
                pthread_mutex_lock(&m_listaReady);
                agregar_lista_ready_con_log(listaReady, pcb_IO,kernel_config.algoritmo_planificacion);
                pthread_mutex_unlock(&m_listaReady);
                sem_post(&proceso_en_ready);
                liberar_ce(contexto_IO);
                break;
            default:
                log_error(kernel_logger, "entro algo que no deberia");
                break;
            //case BLOCK_por_ACCESO_A_MEM: (CREEMOS Q ES UN BLOCK IO )

            // case BLOCK_IO: ;
            //     int tiempo_io_ms = 0; // aca rompe carajoOOOOO, cuando recibe no tiene bien los parametros
            //     t_pcb* pcb_desalojadoIO = recibir_pcb_tiempo_ms(cpu_dispatch_connection, &tiempo_io_ms, kernel_logger);
            //     log_trace(kernel_logger, "el tiempo a bloquear es: %d", tiempo_io_ms);

            //         pthread_mutex_lock(&m_listaEjecutando);
            //     list_remove(listaEjecutando, 0); // inicializar pcb y despues liberarlo
            //         pthread_mutex_unlock(&m_listaEjecutando);
                
            //     sem_post(&cpu_libre_para_ejecutar); // el mas importante jiji
                
            //     pcb_desalojadoIO->rafaga_anterior = pcb_desalojadoIO->sumatoria_rafaga; // Cambiamos T(n-1) -> T(n)
            //     pcb_desalojadoIO->sumatoria_rafaga = 0; // Reseteamos valor de sumatoria 

            //     pcb_desalojadoIO->estimacion_rafaga = estimacion_proxima_rafaga(pcb_desalojadoIO); // EST(n-1) paso a ser EST(n) // TO DO REVISAR ESTO
            //     pcb_desalojadoIO->estimacion_fija = pcb_desalojadoIO->estimacion_rafaga;
                       
            //     Retorno_io* nodo_io = malloc(sizeof(Retorno_io)); 

            //     nodo_io->pcb = pcb_desalojadoIO;
            //     nodo_io->tiempo_io_ms = tiempo_io_ms;
            //     struct timeval llegada;
            //     gettimeofday(&llegada, NULL);                
            //     nodo_io->llegada = llegada.tv_sec;
                
                
            //        pthread_mutex_unlock(&m_io);
            //     list_add(listaIO, nodo_io);
            //        pthread_mutex_unlock(&m_io);

            //     pthread_t hilo_suspension;
            //             //pthread_create(&hiloDispatch, NULL, (void*) manejar_dispatch, (void*)cpu_dispatch_connection);
            //             pthread_create(&hilo_suspension, NULL, (void*)  timer_suspension, (void*) pcb_desalojadoIO   );
            //             pthread_detach(hilo_suspension);

                
            //     sem_post(&llego_io);// hilo!!!
        }
    }
}

int recurso_no_existe(char* recurso)
{ // verifica si no existe el recurso - retorna 0 si existe - 1 si no existe
    int cantidad = string_array_size(kernel_config.recursos);

    for (int i = 0; i < cantidad; i++) {
        if (strcmp((char* )kernel_config.recursos[i], recurso) == 0){
            return 0;
        }
        log_warning(kernel_logger, "%s",kernel_config.recursos[i]);
    }
    return 1;
}

int obtener_id_recurso(char* recurso)
{
    int cantidad = string_array_size(kernel_config.recursos);

    for (int i = 0; i < cantidad; i++) {
        if (strcmp(kernel_config.recursos[i], recurso) == 0){
            return i;
        }
    }
}

void reencolar_bloqueo_por_recurso(int id_recurso)
{
    sem_wait(sem_recurso[id_recurso]);
    pthread_mutex_lock(m_listaRecurso[id_recurso]);
        t_pcb * pcb_a_reencolar = (t_pcb *) list_remove(lista_recurso[id_recurso], 0); // inicializar pcb y despues liberarlo
    pthread_mutex_unlock(m_listaRecurso[id_recurso]);
    cambiar_estado_a(pcb_a_reencolar, READY, estadoActual(pcb_a_reencolar));
    iniciar_nueva_espera_ready(pcb_a_reencolar); // hacer cada vez que se mete en la lista de ready
    pthread_mutex_lock(&m_listaReady);
    agregar_lista_ready_con_log(listaReady, pcb_a_reencolar,kernel_config.algoritmo_planificacion);
    pthread_mutex_unlock(&m_listaReady);
    sem_post(&proceso_en_ready);
}


int tiene_que_reencolar_bloq_recurso(int id_recurso)
{
    return (kernel_config.instancias_recursos[id_recurso] < 0);
}

void sumar_instancia(int id_recurso)
{
    kernel_config.instancias_recursos[id_recurso] += 1;
}

void restar_instancia(int id_recurso){ // resta 1 a la instancia
    kernel_config.instancias_recursos[id_recurso] -= 1;
}

int tiene_instancia_wait(int id_recurso){ // devuelve 1 si instancia recurso es >= 0, 0 en otro caso
    return (kernel_config.instancias_recursos[id_recurso] >= 0);
}

void bloqueo_proceso_en_recurso(t_pcb* pcb, int id_recurso)
{
    pthread_mutex_lock(m_listaRecurso[id_recurso]);
        list_add(lista_recurso[id_recurso], pcb);
    pthread_mutex_unlock(m_listaRecurso[id_recurso]);
}

void actualizar_pcb(t_pcb* pcb, contexto_ejecucion* ce)
{
    copiar_PC_ce_a_pcb(ce, pcb);
    copiar_registros_ce_a_pcb(ce, pcb);
    //falta copiar la tabla de segmentos.
}

void enviar_Fin_consola(int socket)
{
    // pthread_mutex_lock(&mutexOk);
    t_paquete *paquete;
    paquete = crear_paquete_op_code(FIN_CONSOLA);
    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
    liberar_conexion(socket);
}

void sacar_rafaga_ejecutada(t_pcb* pcb)
{
    pcb->rafaga_ejecutada = temporal_gettime(pcb->salida_ejecucion);
}

void iniciar_nueva_espera_ready(t_pcb* pcb)
{
    temporal_destroy(pcb->tiempo_llegada_ready);
    pcb->tiempo_llegada_ready = temporal_create();
}

void agregar_lista_ready_con_log(t_list* listaready,t_pcb* pcb_a_encolar,char* algoritmo){
    list_add(listaready,pcb_a_encolar);

    t_list* lista_pids= list_create();
    lista_pids=list_map(listaReady,(void*)obtenerPid); // t_list con enteros
    char* string_pids = string_new();

    int tamanio = list_size(lista_pids);

    for(int i = 0; i < tamanio; i++){
        char* string_a_agregar = strcat(string_itoa(list_get(lista_pids,i)), " ");
        string_append(&string_pids,string_a_agregar);
    }

    log_info(kernel_logger,"Cola Ready [%s] : [%s]", algoritmo, string_pids);

    list_destroy(lista_pids);
    
}

// funcion dar string -> Id + ID + ID
// ----------------------- Funciones finales ----------------------- //

void destruirSemaforos()
{
    sem_destroy(&proceso_en_ready);
    sem_destroy(&fin_ejecucion);
    sem_destroy(&grado_multiprog);
}



bool bloqueado_termino_io(t_pcb *pcb)
{
    return (pcb->estado_actual == BLOCKED); //VER aca antes era BLOCKED_READY
}

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


TODO:
PID: <PID> - Bloqueado por: <NOMBRE_ARCHIVO>

*/