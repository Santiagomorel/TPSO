#include "kernel.h"

int main(int argc, char **argv)
{
    // ----------------------- creo el log del kernel ----------------------- //

    kernel_logger = init_logger("./runlogs/kernel.log", "KERNEL", 1, LOG_LEVEL_TRACE);

    // ----------------------- levanto y cargo la configuracion del kernel ----------------------- //

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

    iniciar_planificadores();

    // ----------------------- contecto el kernel con los servidores de MEMORIA - CPU (dispatch) - FILESYSTEM ----------------------- //

    if((memory_connection = crear_conexion(kernel_config.ip_memoria , kernel_config.puerto_memoria)) == -1) {
        log_trace(kernel_logger, "No se pudo conectar al servidor de MEMORIA");
        exit(2);
    }
    recibir_operacion(memory_connection);
    recibir_mensaje(memory_connection, kernel_logger);

    if((cpu_dispatch_connection = crear_conexion(kernel_config.ip_cpu , kernel_config.puerto_cpu)) == -1) {
        log_trace(kernel_logger, "No se pudo conectar al servidor de CPU DISPATCH");
        exit(2);
    }
    recibir_operacion(cpu_dispatch_connection);
    recibir_mensaje(cpu_dispatch_connection, kernel_logger);
    // if((file_system_connection = crear_conexion(kernel_config.ip_file_system , kernel_config.puerto_file_system)) == -1) {
    //     log_trace(kernel_logger, "No se pudo conectar al servidor de FILE SYSTEM");
    //     exit(2);
    // }

    // ----------------------- levanto el servidor del kernel ----------------------- //

    socket_servidor_kernel = iniciar_servidor(kernel_config.puerto_escucha, kernel_logger);
    log_trace(kernel_logger, "kernel inicia el servidor");

    // ----------------------- espero conexiones de consola ----------------------- //

    while (1)
    {

        log_trace(kernel_logger, "esperando cliente consola");
        socket_cliente = esperar_cliente(socket_servidor_kernel, kernel_logger);
        log_trace(kernel_logger, "entro una consola con el socket: %d", socket_cliente);

        pthread_t atiende_consola;
        pthread_create(&atiende_consola, NULL, (void *)recibir_consola, (void *)socket_cliente);
        pthread_detach(atiende_consola);
    }

    return EXIT_SUCCESS;
}

// ----------------------- Funciones Globales de Kernel ----------------------- //


void load_config(void)
{

    kernel_config.ip_memoria = config_get_string_value(kernel_config_file, "IP_MEMORIA");
    kernel_config.puerto_memoria = config_get_string_value(kernel_config_file, "PUERTO_MEMORIA");
    kernel_config.ip_file_system = config_get_string_value(kernel_config_file, "IP_FILESYSTEM");
    kernel_config.puerto_file_system = config_get_string_value(kernel_config_file, "PUERTO_FILESYSTEM");
    kernel_config.ip_cpu = config_get_string_value(kernel_config_file, "IP_CPU");
    kernel_config.puerto_cpu = config_get_string_value(kernel_config_file, "PUERTO_CPU");
    kernel_config.puerto_escucha = config_get_string_value(kernel_config_file, "PUERTO_ESCUCHA");
    kernel_config.algoritmo_planificacion = config_get_string_value(kernel_config_file, "ALGORITMO_CLASIFICACION");

    kernel_config.estimacion_inicial = config_get_int_value(kernel_config_file, "ESTIMACION_INICIAL");

    kernel_config.hrrn_alfa = config_get_long_value(kernel_config_file, "HRRN_ALFA");

    kernel_config.grado_max_multiprogramacion = config_get_int_value(kernel_config_file, "GRADO_MAX_MULTIPROGRAMACION");

    kernel_config.recursos = config_get_string_value(kernel_config_file, "RECURSOS");
    kernel_config.instancias_recursos = config_get_string_value(kernel_config_file, "INSTANCIAS_RECURSOS");

    log_trace(kernel_logger, "config cargada en 'kernel_cofig_file'");
}

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
        log_info(kernel_logger, "Se crea el proceso %d en NEW", pcb_a_iniciar->id);

        enviar_mensaje("Llego el paquete", SOCKET_CLIENTE);

        planificar_sig_to_ready(); // usar esta funcion cada vez q se agregue un proceso a NEW o BLOCKED
        break;

    default:
        log_trace(kernel_logger, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
        break;
    }
}

t_pcb *iniciar_pcb(int socket)
{
    int size = 0;

    char *buffer;
    int desp = 0;

    buffer = recibir_buffer(&size, socket);

    char *instrucciones = leer_string(buffer, &desp);
    // int tamanio_proceso = leer_entero(buffer, &desp);        //TODO aca tengo que leer la cantidad de lineas de pseudocodigo

    t_pcb *nuevo_pcb = pcb_create(instrucciones, socket);
    free(instrucciones);
    free(buffer);

    loggear_pcb(nuevo_pcb, kernel_logger);

    return nuevo_pcb;
}

t_pcb *pcb_create(char *instrucciones, int socket_consola)
{
    log_trace(kernel_logger, "instrucciones en pcb create %s", instrucciones);
    t_pcb *new_pcb = malloc(sizeof(t_pcb));

    generar_id(new_pcb);
    new_pcb->estado_actual = NEW;
    new_pcb->instrucciones = separar_inst_en_lineas(instrucciones);
    new_pcb->program_counter = 0;
    // new_pcb->tabla_paginas = -1; // TODO de la conexion con memoria
    new_pcb->estimacion_rafaga = kernel_config.estimacion_inicial; // ms
    new_pcb->socket_consola = socket_consola;
    new_pcb->tabla_segmentos = list_create();

    return new_pcb;
}

void generar_id(t_pcb *pcb)
{ // Con esta funcion le asignamos a un pcb su id, cuidando de no repetir el numero
    pthread_mutex_lock(&m_contador_id);
    pcb->id = contador_id;
    contador_id++;
    pthread_mutex_unlock(&m_contador_id);
}

char **separar_inst_en_lineas(char *instrucciones)
{
    char **lineas = parsearPorSaltosDeLinea(instrucciones);

    char **lineas_mejorado = string_array_new();
    int i;

    for (i = 0; i < string_array_size(lineas); i++)
    {

        char **instruccion = string_split(lineas[i], " "); // separo por espacios
        log_trace(kernel_logger, "Agrego la linea '%s' a las intrucciones del pcb", lineas[i]);
        // log_info(logger_kernel, "instruccion[0] es: %s y la instruccion[1] es:%s", instruccion[0], instruccion[1]);
        string_array_push(&lineas_mejorado, string_duplicate(lineas[i]));
    }
    string_array_destroy(lineas);
    return lineas_mejorado;
}

char **parsearPorSaltosDeLinea(char *buffer)
{

    char **lineas = string_split(buffer, "\n");

    return lineas;
}

void inicializarListasGlobales(void)
{

    listaNuevos = list_create();
    listaReady = list_create();
    listaBloqueados = list_create();
    listaEjecutando = list_create();
    listaFinalizados = list_create();

    listaIO = list_create();
}

void iniciar_planificadores()
{
    //pthread_create(&planificadorCP, NULL, (void *)planificar_sig_to_running, (void *)socket_cliente);
    //pthread_detach(planificadorCP);
    // MODIFICAR cuando haga falta hacer el running

}

void iniciarSemaforos()
{
    pthread_mutex_init(&m_listaNuevos, NULL);
    pthread_mutex_init(&m_listaReady, NULL);
    pthread_mutex_init(&m_listaBloqueados, NULL);
    pthread_mutex_init(&m_listaEjecutando, NULL);
    pthread_mutex_init(&m_contador_id, NULL);
    sem_init(&proceso_en_ready, 0, 0);
    sem_init(&grado_multiprog, 0, kernel_config.grado_max_multiprogramacion);
}

void destruirSemaforos()
{
    sem_destroy(&proceso_en_ready);
}

void planificar_sig_to_running(){

    while(1){
        sem_wait(&proceso_en_ready);

        if(kernel_config.algoritmo_planificacion == "FIFO") { // FIFO
            //log_info(kernel_logger, "Cola Ready FIFO: %s", funcionQueMuestraPID()); // HACER // VER LOCALIZACION
            pthread_mutex_lock(&m_listaReady);
            t_pcb* pcb_a_ejecutar = list_remove(listaReady, 0);
            pthread_mutex_unlock(&m_listaReady);

            log_trace(kernel_logger, "agrego a RUNING y se lo paso a cpu para q ejecute!");

            cambiar_estado_a(pcb_a_ejecutar, RUNNING, estadoActual(pcb_a_ejecutar));
            agregar_a_lista_con_sems(pcb_a_ejecutar, listaEjecutando, m_listaEjecutando);

            enviar_ce(cpu_dispatch_connection, pcb_a_ejecutar, EJECUTAR_PCB);
            //eliminar_pcb(pcb_a_ejecutar)??
        }
        else if (kernel_config.algoritmo_planificacion == "HRRN"){
             pthread_mutex_lock(&m_listaReady);
              //log_info(kernel_logger,"HRRN: Hay %d procesos listos para ejecutar",list_size(listaReady);
            t_pcb* pcb_mayorRR = list_get_maximum(listaReady,(void*) mayorRRdeLista);
            t_pcb* pcb_a_ejecutar = list_remove_element(listaReady, pcb_mayorRR);
             pthread_mutex_unlock(&m_listaReady,pcb_mayorRR);
            log_trace(kernel_logger, "agrego a RUNING y se lo paso a cpu para q ejecute!");

            cambiar_estado_a(pcb_a_ejecutar, RUNNING, estadoActual(pcb_a_ejecutar));
            agregar_a_lista_con_sems(pcb_a_ejecutar, listaEjecutando, m_listaEjecutando);

            enviar_ce(cpu_dispatch_connection, pcb_a_ejecutar, EJECUTAR_PCB);

        }

    }
}
void planificar_sig_to_ready()
{

    sem_wait(&grado_multiprog);

    if (!list_is_empty(listaBloqueados) && list_any_satisfy(listaBloqueados, bloqueado_termino_io))
    { // BLOCKED -> READY

        pthread_mutex_lock(&m_listaBloqueados);
        t_pcb *pcb_a_ready = list_remove_by_condition(listaBloqueados, bloqueado_termino_io);
        pthread_mutex_unlock(&m_listaBloqueados);
        // PRESCENCIA=1
        // pcb_a_ready->tabla_paginas = pedir_tabla_pags(pcb_a_ready, conexion_memoria); // TODO pedir_tabla_pags
        
        cambiar_estado_a(pcb_a_ready, READY, estadoActual(pcb_a_ready));
        agregar_a_lista_con_sems(pcb_a_ready, listaReady, m_listaReady);

        sem_post(&proceso_en_ready);
    }
    else if (!list_is_empty(listaNuevos)) // NEW -> READY
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
        cambiar_estado_a(pcb_a_ready, READY, estadoActual(pcb_a_ready));                            // NO ESTABA
        agregar_a_lista_con_sems(pcb_a_ready, listaReady, m_listaReady); // NO ESTABA

        sem_post(&proceso_en_ready);
        // log_trace(kernel_logger, "entro el primer pcb a READY!");
    }
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

int estadoActual(t_pcb * pcb) //VER
{
    return pcb->estado_actual;
}

bool bloqueado_termino_io(t_pcb *pcb)
{
    return (pcb->estado_actual == BLOCKED); //VER aca antes era BLOCKED_READY
}

void cambiar_estado_a(t_pcb *a_pcb, estados nuevo_estado, estados estado_anterior)
{
    a_pcb->estado_actual = nuevo_estado;
    log_info(kernel_logger,"Cambio de Estado: PID: %d - Estado Anterior: %s - Estado Actual: %s", obtenerPid(a_pcb), obtenerEstado(estado_anterior), obtenerEstado(nuevo_estado));
}

int obtenerPid(t_pcb * pcb)
{
    return pcb->id;
}

char * obtenerEstado(estados estado)
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

void agregar_a_lista_con_sems(t_pcb *pcb_a_agregar, t_list *lista, pthread_mutex_t m_sem)
{
    pthread_mutex_lock(&m_sem);
    list_add(lista, pcb_a_agregar);
    pthread_mutex_unlock(&m_sem);
}

void inicializar_estructuras(t_pcb *pcb)
{
    t_paquete *paquete = crear_paquete_op_code(INICIAR_ESTRUCTURAS);
    agregar_entero_a_paquete(paquete, pcb->id);

    enviar_paquete(paquete, memory_connection);

    eliminar_paquete(paquete);
}

void pedir_tabla_segmentos() // MODIFICAR tipo de dato que devuelve
{
    int codigoOperacion = recibir_operacion(memory_connection);
    if (codigoOperacion != MENSAJE) //TABLA_SEGMENTOS MODIFICAR cuando tengamos la tabla de segmentos
    {
        log_trace(kernel_logger, "llego otra cosa q no era un tabla pags :c");
    }
    recibir_mensaje(memory_connection, kernel_logger);
    //return recibir_paquete(memory_connection);
}

t_pcb* mayorRRdeLista ( void* _pcb1,void* _pcb2){

        t_pcb* pcb1 = (t_pcb*)_pcb1;

        t_pcb* pcb2 = (t_pcb*)_pcb2;

        return mayorRR(pcb1,pcb2);

    };


time_t calculoRR (time_t tiempoEsperaEnReady,time_t duracionRealAnterior,time_t estimacionAnterior){

    return 1 +( ( (time(NULL) - tiempoEsperaEnReady) *1000) /calculoEstimado(duracionRealAnterior,estimacionAnterior) );

}

double calculoEstimado (time_t duracionRealAnterior,time_t estimacionAnterior){

    double alfa = kernel_config.hrrn_alfa;

    return (alfa * duracionRealAnterior) + ( (1 - alfa) * estimacionAnterior) ;

}

t_pcb* mayorRR (t_pcb* pcb1,t_pcb* pcb2){

    int RR_pcb1 = calculoRR(pcb1->instanteEstado,pcb1->duracionRealAnterior,pcb1->estimacionAnterior);

    int RR_pcb2 = calculoRR(pcb2->instanteEstado,pcb2->duracionRealAnterior,pcb2->estimacionAnterior);

    //log_info(logger,"Comparo pcb1 [%d] y pcb2[%d], RR_pcb1 [%d] y RR_pcb2 [%d] ",pcb1->pid, pcb2->pid,RR_pcb1,RR_pcb2 );

    if (RR_pcb1 >= RR_pcb2) return pcb1;

    else return pcb2;

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
// recibir_mensaje(socket_cliente,kernel_logger);

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