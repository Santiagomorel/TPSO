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
    // for(int i = 0; i < 3; i++) // tirar 2 consolas, esperar a que terminen, luego tirar la 3ra para cortar el programa
    while (1)
    {
        //log_trace(kernel_logger, "esperando cliente consola");
        socket_cliente = esperar_cliente(socket_servidor_kernel, kernel_logger);
        log_trace(kernel_logger, "entro una consola con el socket: %d", socket_cliente);

        pthread_t atiende_consola;
        pthread_create(&atiende_consola, NULL, (void *)recibir_consola, (void *) (intptr_t) socket_cliente);
        pthread_detach(atiende_consola);
    }

    // falta liberar instancias de sockets, semaforos, free intArray de las instancias, free listas_recursos
    // semaforos de los recursos
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
    tablaGlobalArchivosAbiertos = list_create();
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
    pthread_mutex_init(&m_IO, NULL);
    pthread_mutex_init(&m_F_operation, NULL);

    sem_init(&proceso_en_ready, 0, 0);
    sem_init(&fin_ejecucion, 0, 1);
    sem_init(&grado_multiprog, 0, kernel_config.grado_max_multiprogramacion);
    iniciar_semaforos_recursos(kernel_config.recursos, kernel_config.instancias_recursos);
}

void iniciar_semaforos_recursos(char** recursos, int* instancias_recursos)
{
    int cantidad_recursos = string_array_size(recursos);

    if (cantidad_instancias != cantidad_recursos) {
        log_error(kernel_logger, "La cantidad de instancias de recursos es diferente a la cantidad de recursos en config");
        exit(1);
    }

    for (int i = 0; i < cantidad_recursos; i++) {
        m_listaRecurso[i] = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)); // VER luego hay que eliminar los malloc
        pthread_mutex_init(m_listaRecurso[i], NULL);
    }

    for (int i = 0; i < cantidad_recursos; i++) {
        sem_recurso[i] = (sem_t*)malloc(sizeof(sem_t)); // VER luego hay que eliminar los malloc
        sem_init(sem_recurso[i], 0, instancias_recursos[i]);
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

    // loggear_pcb(nuevo_pcb, kernel_logger);

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
    new_pcb->tabla_archivos_abiertos_por_proceso = list_create();
    
    new_pcb->salida_ejecucion = temporal_create();
    new_pcb->rafaga_ejecutada = 0;
    
    new_pcb->calculoRR = 1;

    new_pcb->socket_consola = socket_consola;
    new_pcb->estado_actual = NEW;

    new_pcb-> recursos_pedidos = list_create();
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

uint32_t obtenerPid(t_pcb* pcb)
{
    return pcb->id;
}

char* obtener_nombre_archivo(t_entradaTGAA* entrada){
    return entrada->nombreArchivo;
}


bool existeArchivo(char* nombreArchivo){
  
    //t_list* nombreArchivosAbiertos = list_map(tablaGlobalArchivosAbiertos,(void*) obtener_nombre_archivo);
    // log_trace(kernel_logger, "La lista de nombres es de tamanio: %d", list_size(nombreArchivosAbiertos));
    log_warning(kernel_logger, "La lista de nombres es de tamanio, con el list size: %d", list_size(tablaGlobalArchivosAbiertos));
    if(list_size(tablaGlobalArchivosAbiertos)){
    t_entradaTGAA* aux = list_get(tablaGlobalArchivosAbiertos,0);
    log_error(kernel_logger, "Aux tiene: %s de nombre y %d de tamanio", aux->nombreArchivo,aux->tamanioArchivo);
    }
    t_list* archivoEnLista = nombre_en_lista_coincide(tablaGlobalArchivosAbiertos,nombreArchivo);
    // ver si se puede utilizar (de la commons) list_any_satisfy para comprobar si existe un archivo.
    log_error(kernel_logger, "La lista de nombres es de tamanio: %d", list_size(archivoEnLista));
    if(list_size(archivoEnLista)){
        t_entradaTGAA* entradaGlobal = list_get(archivoEnLista,0);
        log_error(kernel_logger, "el nombre del archivo es: %s",entradaGlobal->nombreArchivo);
        return strcmp(entradaGlobal->nombreArchivo, nombreArchivo) == 0;
    }else{
        return 0;
    }
     
 
}

t_entradaTGAA* obtenerEntrada(char* nombreArchivo){
    
    bool buscarArchivo(t_entradaTGAA* entrada){
    return strcmp(entrada->nombreArchivo, nombreArchivo) == 0;
    }

    return list_find(tablaGlobalArchivosAbiertos,(void*) buscarArchivo);
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

int estadoActual(t_pcb* pcb)
{
    return pcb->estado_actual;
}

void agregar_a_lista_con_sems(t_pcb* pcb_a_agregar, t_list* lista, pthread_mutex_t m_sem)
{
    pthread_mutex_lock(&m_sem);
        agregar_lista_ready_con_log(lista, pcb_a_agregar, kernel_config.algoritmo_planificacion);
    pthread_mutex_unlock(&m_sem);
}

void agregar_lista_ready_con_log(t_list* listaready,t_pcb* pcb_a_encolar,char* algoritmo){
    list_add(listaready,pcb_a_encolar);

    t_list* lista_pids = list_create();
    lista_pids = list_map(listaReady, (void*) obtenerPid);
    char* string_pids = string_new();

    int tamanio = list_size(lista_pids);
    if (tamanio >0) {
        char* ultimo_string_a_agregar = string_itoa((int) list_get(lista_pids, (tamanio - 1)));

        for(int i = 0; i < (tamanio - 1); i++){
            char* string_a_agregar = strcat(string_itoa((int) list_get(lista_pids, i)), " ");
            string_append(&string_pids, string_a_agregar);
        }

        string_append(&string_pids, ultimo_string_a_agregar);

        log_info(kernel_logger,"Cola Ready [%s] : [%s]", algoritmo, string_pids);

        //list_destroy(lista_pids);
    }
    else {
        log_info(kernel_logger,"Cola Ready [%s] : [%s]", algoritmo, string_pids);

        //list_destroy(lista_pids);
    }
    list_destroy(lista_pids);
    free(string_pids);
}

t_pcb* actualizar_pcb_lget_devuelve_pcb(contexto_ejecucion* contexto_actualiza, t_list* lista_del_pcb, pthread_mutex_t sem) // actualiza un pcb en base a su id de una lista y lo devuelve
{
    pthread_mutex_lock(&sem);
        t_pcb * pcb_a_actualizar = pcb_en_lista_coincide(lista_del_pcb, contexto_actualiza->id);
        actualizar_pcb(pcb_a_actualizar, contexto_actualiza);
    pthread_mutex_unlock(&sem);

    return pcb_a_actualizar;
} // probar que funcione

t_pcb* pcb_lget_devuelve_pcb(contexto_ejecucion* contexto_actualiza, t_list* lista_del_pcb, pthread_mutex_t sem) // devuelve un pcb en base a su id de una lista
{
    pthread_mutex_lock(&sem);
        t_pcb * pcb_a_devolver = pcb_en_lista_coincide(lista_del_pcb, contexto_actualiza->id);
    pthread_mutex_unlock(&sem);

    return pcb_a_devolver;
}

t_pcb* actualizar_pcb_lremove_devuelve_pcb(contexto_ejecucion* contexto_actualiza, t_list* lista_del_pcb, pthread_mutex_t sem) // actualiza y remueve un pcb en base a su id de una lista y lo devuelve
{
    pthread_mutex_lock(&sem);
        t_pcb * pcb_a_actualizar = pcb_en_lista_coincide(lista_del_pcb, contexto_actualiza->id);
        bool valor_retorno_de_eliminacion_elemento = list_remove_element(lista_del_pcb, pcb_a_actualizar);
        log_debug(kernel_logger, "el valor que me da cuando intento eliminar el elemento con id %d es %d", pcb_a_actualizar->id, valor_retorno_de_eliminacion_elemento);
        actualizar_pcb(pcb_a_actualizar, contexto_actualiza);
    pthread_mutex_unlock(&sem);

    return pcb_a_actualizar;
}

t_pcb* pcb_lremove(contexto_ejecucion* contexto_actualiza, t_list* lista_del_pcb, pthread_mutex_t sem) // remueve un pcb en base a su id de una lista y lo devuelve
{
    pthread_mutex_lock(&sem);
        t_pcb * pcb_coincide_con_id = pcb_en_lista_coincide(lista_del_pcb, contexto_actualiza->id);
        list_remove_element(lista_del_pcb, pcb_coincide_con_id);
    pthread_mutex_unlock(&sem);

    return pcb_coincide_con_id;
}
// ----------------------- Funciones planificador to - ready ----------------------- //

void planificar_sig_to_ready()
{
    sem_wait(&grado_multiprog);

    if (!list_is_empty(listaNuevos)) // NEW -> READY
    {
        pthread_mutex_lock(&m_listaNuevos);
            t_pcb *pcb_a_ready = list_remove(listaNuevos, 0);
        pthread_mutex_unlock(&m_listaNuevos);

        log_trace(kernel_logger, "Inicializamos estructuras del pcb en memoria");
        
        inicializar_estructuras(pcb_a_ready);

        t_list* nuevo_segmento = pedir_tabla_segmentos();

        list_add_all(pcb_a_ready->tabla_segmentos, nuevo_segmento);

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

t_list* pedir_tabla_segmentos()
{
    int codigoOperacion = recibir_operacion(memory_connection);
    if (codigoOperacion != TABLA_SEGMENTOS)
    {
        log_error(kernel_logger, "Perdir tabla de segmentos no recibio una Tabla");
    }

    uint32_t size;

    recv(memory_connection, &size, sizeof(uint32_t), NULL);

    void* buffer = malloc(size * sizeof(t_ent_ts));

    recv(memory_connection, buffer, size * sizeof(t_ent_ts), NULL);

    t_list* tabla_de_segmentos = deserializar_tabla_segmentos(buffer, size);

    free(buffer);

    return tabla_de_segmentos;
}

// ----------------------- Funciones planificador to - running ----------------------- //

void planificar_sig_to_running()
{
    while(1){
        sem_wait(&fin_ejecucion); // hacer un post cada vez que un proceso deje de ejecutar
        sem_wait(&proceso_en_ready);
        log_trace(kernel_logger, "Entra en la planificacion de READY RUNNING");
        if(strcmp(kernel_config.algoritmo_planificacion, "FIFO") == 0) { // FIFO
            pthread_mutex_lock(&m_listaReady);
                t_pcb* pcb_a_ejecutar = list_remove(listaReady, 0);
            pthread_mutex_unlock(&m_listaReady);
            funcion_agregar_running(pcb_a_ejecutar);
        }
        else if (strcmp(kernel_config.algoritmo_planificacion, "HRRN") == 0){ // HRRN
            uint32_t tamanioLista = list_size(listaReady);
            if(tamanioLista == 1){
                log_trace(kernel_logger, "El tamanio de la lista de ready es 1, hago FIFO");
                pthread_mutex_lock(&m_listaReady);
                    t_pcb* pcb_a_ejecutar = list_remove(listaReady, 0);
                pthread_mutex_unlock(&m_listaReady);
                funcion_agregar_running(pcb_a_ejecutar);
            }else{
                log_trace(kernel_logger, "El tamanio de la lista de ready es MAYOR");
                t_pcb* pcb_a_ejecutar = list_get_maximum(listaReady, (void*) mayorRRdeLista);
                setear_estimacion(pcb_a_ejecutar);
                pthread_mutex_lock(&m_listaReady);
                    list_remove_element(listaReady, pcb_a_ejecutar);
                pthread_mutex_unlock(&m_listaReady);
                funcion_agregar_running(pcb_a_ejecutar);
            }
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

t_pcb* mayorRR(t_pcb* pcb1, t_pcb* pcb2) // Retorna el mayor Response Ratio entre 2 procesos
{
    double RR_pcb1 = calcularRR(pcb1);

    double RR_pcb2 = calcularRR(pcb2);

    log_trace(kernel_logger,"Comparo pcb1 [%d] y pcb2 [%d], RR_pcb1 [%ld] y RR_pcb2 [%ld] ", pcb1->id, pcb2->id, RR_pcb1, RR_pcb2);

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
        //log_trace(kernel_logger, "El proceso PID %d calcula RR CON CALCULO, rafaga e. de %ld ms", pcb->id, pcb->rafaga_ejecutada);
        double calculo = calculoEstimado(pcb);
        double valorRetorno = round(1 + (tiempoEspera/(calculo/1000)));
        pcb->calculoRR = valorRetorno;
        return valorRetorno;
    } else {
        //log_trace(kernel_logger, "El proceso PID %d calcula RR ESTIMACION INICIAL", pcb->id);
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

// ----------------------- Funciones CE ----------------------- //

contexto_ejecucion* obtener_ce(t_pcb* pcb)
{
    contexto_ejecucion * nuevoContexto = malloc(sizeof(contexto_ejecucion));
    nuevoContexto->instrucciones = string_array_new();
    nuevoContexto->registros_cpu = malloc(sizeof(t_registro));
    // log_trace(kernel_logger, "hace maloc del nuevo contexto");

    copiar_id_pcb_a_ce(pcb, nuevoContexto);
    // log_trace(kernel_logger, "copia el id del pcb");

    copiar_instrucciones_pcb_a_ce(pcb, nuevoContexto);
    // log_trace(kernel_logger, "copia instrucciones del pcb");

    copiar_PC_pcb_a_ce(pcb, nuevoContexto);
    // log_trace(kernel_logger, "copia program counter");

    copiar_registros_pcb_a_ce(pcb, nuevoContexto);
    // log_trace(kernel_logger, "copia registros del pcb");

    copiar_tabla_segmentos_pcb_a_ce(pcb, nuevoContexto);
    // log_trace(kernel_logger, "copia tabla segmentos del pcb");

    return nuevoContexto;
}

void copiar_id_pcb_a_ce(t_pcb* pcb, contexto_ejecucion* ce)
{
    ce->id = pcb->id;
}

void copiar_instrucciones_pcb_a_ce(t_pcb* pcb, contexto_ejecucion* ce)
{
    for (int i = 0; i < string_array_size(pcb->instrucciones); i++) {
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
    strncpy(ce->registros_cpu->AX , pcb->registros_cpu->AX, 4);
    strncpy(ce->registros_cpu->BX , pcb->registros_cpu->BX, 4);
    strncpy(ce->registros_cpu->CX , pcb->registros_cpu->CX, 4);
    strncpy(ce->registros_cpu->DX , pcb->registros_cpu->DX, 4);
	strncpy(ce->registros_cpu->EAX , pcb->registros_cpu->EAX, 8);
	strncpy(ce->registros_cpu->EBX , pcb->registros_cpu->EBX, 8);
	strncpy(ce->registros_cpu->ECX , pcb->registros_cpu->ECX, 8);
	strncpy(ce->registros_cpu->EDX , pcb->registros_cpu->EDX, 8);
	strncpy(ce->registros_cpu->RAX , pcb->registros_cpu->RAX, 16);
	strncpy(ce->registros_cpu->RBX , pcb->registros_cpu->RBX, 16);
	strncpy(ce->registros_cpu->RCX , pcb->registros_cpu->RCX, 16);
	strncpy(ce->registros_cpu->RDX , pcb->registros_cpu->RDX, 16);
}

void copiar_registros_ce_a_pcb(contexto_ejecucion* ce, t_pcb* pcb)
{
    strncpy(pcb->registros_cpu->AX , ce->registros_cpu->AX, 4);
    strncpy(pcb->registros_cpu->BX , ce->registros_cpu->BX, 4);
    strncpy(pcb->registros_cpu->CX , ce->registros_cpu->CX, 4);
    strncpy(pcb->registros_cpu->DX , ce->registros_cpu->DX, 4);
	strncpy(pcb->registros_cpu->EAX , ce->registros_cpu->EAX, 8);
	strncpy(pcb->registros_cpu->EBX , ce->registros_cpu->EBX, 8);
	strncpy(pcb->registros_cpu->ECX , ce->registros_cpu->ECX, 8);
	strncpy(pcb->registros_cpu->EDX , ce->registros_cpu->EDX, 8);
	strncpy(pcb->registros_cpu->RAX , ce->registros_cpu->RAX, 16);
	strncpy(pcb->registros_cpu->RBX , ce->registros_cpu->RBX, 16);
	strncpy(pcb->registros_cpu->RCX , ce->registros_cpu->RCX, 16);
	strncpy(pcb->registros_cpu->RDX , ce->registros_cpu->RDX, 16);
}

void copiar_tabla_segmentos_pcb_a_ce(t_pcb* pcb, contexto_ejecucion* ce)
{
    ce->tabla_segmentos = list_duplicate(pcb->tabla_segmentos);
}

// ----------------------- Funciones Dispatch Manager ----------------------- //
void manejar_dispatch()
{
    //log_trace(kernel_logger, "Entre por manejar dispatch");
    while(1){
        int cod_op = recibir_operacion(cpu_dispatch_connection);
        switch(cod_op){
            case SEG_FAULT:
                // recuperacion de waits pedidos - archivos
            case SUCCESS:
                atender_final_proceso(cod_op);
                break;

            case DESALOJO_YIELD:
                atender_desalojo_yield();
                break;

            case WAIT_RECURSO:
                atender_wait_recurso();
                break; 

            case SIGNAL_RECURSO:
                atender_signal_recurso();
                break;

            case BLOCK_IO:
                atender_block_io();
                break;

            case CREAR_SEGMENTO:
                atender_crear_segmento();
                break;

            case BORRAR_SEGMENTO:
                atender_borrar_segmento();
                break;

            case ABRIR_ARCHIVO:
                atender_apertura_archivo();
                log_trace(kernel_logger, "Ya atendi apertura");
                break;
            
            case CERRAR_ARCHIVO:
                atender_cierre_archivo();
                log_trace(kernel_logger, "Ya atendi cierre");
                break;

            case ACTUALIZAR_PUNTERO:
                atender_actualizar_puntero();
                break;

            case LEER_ARCHIVO:
                log_trace(kernel_logger, "Ya por fread");
                atender_lectura_archivo();
                log_trace(kernel_logger, "Ya atendi fread");
                break;
            
            case ESCRIBIR_ARCHIVO:
                atender_escritura_archivo();
                log_trace(kernel_logger, "Ya atendi fwrite");
                break;

            case MODIFICAR_TAMAÑO_ARCHIVO:
                atender_modificar_tamanio_archivo();
                log_trace(kernel_logger, "Ya atendi truncar");
                break;

            case -1:
                break;

            default:
                log_error(kernel_logger, "entro algo que no deberia");
                break;
            //case BLOCK_por_ACCESO_A_MEM: (CREEMOS Q ES UN BLOCK IO )
        }
    }
}

void actualizar_pcb(t_pcb* pcb, contexto_ejecucion* ce) //TODO
{   
    copiar_PC_ce_a_pcb(ce, pcb);
    copiar_registros_ce_a_pcb(ce, pcb);
    //falta copiar la tabla de segmentos.
}

// ----------------------- Funciones de final de proceso ----------------------- //

void atender_final_proceso(int cod_op)
{
    // TODO es probable que necesite liberar las instancias en memoria y en file system antes de encolar en EXIT
    contexto_ejecucion* contexto_a_finalizar = recibir_ce(cpu_dispatch_connection);

    finalizar_proceso(contexto_a_finalizar, cod_op);

    liberar_ce(contexto_a_finalizar);
}

void finalizar_proceso(contexto_ejecucion* ce, int cod_op) // Saca de la lista de ejecucion, actualiza el pcb, y finaliza el proceso
{
    t_pcb* pcb_a_finalizar = actualizar_pcb_lremove_devuelve_pcb(ce, listaEjecutando, m_listaEjecutando);

    cambiar_estado_a(pcb_a_finalizar, EXIT, estadoActual(pcb_a_finalizar));

    liberar_memoria(pcb_a_finalizar);

    liberar_archivos_abiertos(pcb_a_finalizar);

    liberar_recursos_pedidos(pcb_a_finalizar);
    
    sem_post(&grado_multiprog);
    sem_post(&fin_ejecucion);

    agregar_a_lista_con_sems(pcb_a_finalizar, listaFinalizados, m_listaFinalizados);
    
    log_info(kernel_logger, "Finaliza el proceso [%d] - Motivo: [%s]", pcb_a_finalizar->id, obtenerCodOP(cod_op));
    
    enviar_Fin_consola(pcb_a_finalizar->socket_consola);
}

void liberar_memoria(t_pcb* pcb)
{
    t_list* segmentos_activos = list_filter(pcb->tabla_segmentos, segmento_activo);
    for (int i = 0; i < list_size(segmentos_activos); i++)
    {
        t_ent_ts* seg = list_get(segmentos_activos, i);
        printf("SEG: %d, BASE: %d, TAM: %d, ACTIVO: %d\n", seg->id_seg, seg->base, seg->tam, seg->activo);
        solicitar_liberacion_segmento(seg->base, seg->tam, pcb->id, seg->id_seg);
    }
    list_destroy_and_destroy_elements(pcb->tabla_segmentos, (void*) free);
    list_destroy(segmentos_activos);
}

bool segmento_activo(t_ent_ts *seg) {
    return seg->activo && (seg->id_seg != 0);
}

void solicitar_liberacion_segmento(uint32_t base, uint32_t tam, uint32_t pid, uint32_t seg_id)
{
    t_paquete* paquete = crear_paquete_op_code(DELETE_SEGMENT);

    agregar_entero_a_paquete(paquete, pid); 
    agregar_entero_a_paquete(paquete, seg_id); 
	agregar_entero_a_paquete(paquete, base);
    agregar_entero_a_paquete(paquete, tam);

    enviar_paquete(paquete, memory_connection);

    eliminar_paquete(paquete);

    log_error(kernel_logger, "PID: %d - Eliminar Segmento - Id: %d - Tamaño: %d", pid, seg_id, tam);
}

void liberar_archivos_abiertos(t_pcb*)
{
    log_error(kernel_logger, "Elimino los archivos abiertos del proceso que entra en EXIT");
}


void liberar_recursos_pedidos(t_pcb* pcb)
{
    for (int i = 0; i < list_size(pcb->recursos_pedidos); i++)
    {
        log_warning(kernel_logger, "El proceso con id: %d tiene %d recursos pedidos", pcb->id, list_size(pcb->recursos_pedidos));
        log_warning(kernel_logger, "se libera un recurso al finalizar el proceso");
        int element = list_get(pcb->recursos_pedidos, i);
        sumar_instancia_exit(element, pcb);
        log_warning(kernel_logger, "se suma la instancia");
        if (tiene_que_reencolar_bloq_recurso(element)) {
            log_warning(kernel_logger, "se detecto un proceso a reencolar");
            reencolar_bloqueo_por_recurso(element);
        }
    }
    list_clean(pcb->recursos_pedidos);
}

void enviar_Fin_consola(int socket)
{
    t_paquete *paquete;
    paquete = crear_paquete_op_code(FIN_CONSOLA);
    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
    liberar_conexion(socket);
}

// ----------------------- Funciones DESALOJO_YIELD ----------------------- //

void atender_desalojo_yield()
{
    contexto_ejecucion* contexto_a_reencolar = recibir_ce(cpu_dispatch_connection);

    t_pcb* pcb_a_reencolar = actualizar_pcb_lremove_devuelve_pcb(contexto_a_reencolar, listaEjecutando, m_listaEjecutando);

    // pthread_mutex_lock(&m_listaEjecutando);
    //     t_pcb * pcb_a_reencolar = (t_pcb *) list_remove(listaEjecutando, 0); // inicializar pcb y despues liberarlo
    //     actualizar_pcb(pcb_a_reencolar, contexto_a_reencolar);
    // pthread_mutex_unlock(&m_listaEjecutando);

    cambiar_estado_a(pcb_a_reencolar, READY, estadoActual(pcb_a_reencolar));

    sacar_rafaga_ejecutada(pcb_a_reencolar); // hacer cada vez que sale de running
    iniciar_nueva_espera_ready(pcb_a_reencolar); // hacer cada vez que se mete en la lista de ready
    
    agregar_a_lista_con_sems(pcb_a_reencolar, listaReady, m_listaReady);

    sem_post(&proceso_en_ready);
    sem_post(&fin_ejecucion);

    liberar_ce(contexto_a_reencolar);
    //eliminar(pcb_a_reencolar);
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

// ----------------------- Funciones Manejo de Recursos ----------------------- //

uint32_t recurso_no_existe(char* recurso)
{ // verifica si no existe el recurso - retorna 0 si existe - 1 si no existe
    int cantidad = string_array_size(kernel_config.recursos);

    for (int i = 0; i < cantidad; i++) {
        if (strcmp((char* )kernel_config.recursos[i], recurso) == 0){
            return 0;
        }
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

uint32_t obtener_instancias_recurso(int id_recurso)
{
    return kernel_config.instancias_recursos[id_recurso];
}

void restar_instancia(int id_recurso, contexto_ejecucion* contexto)
{ // resta 1 a la instancia y se la asigno a recursos_pedidos del proceso en ejecucion
    kernel_config.instancias_recursos[id_recurso] -= 1;

    t_pcb * pcb_pide_recurso = pcb_lget_devuelve_pcb(contexto, listaEjecutando, m_listaEjecutando);

    pthread_mutex_lock(&m_listaEjecutando);
        list_add(pcb_pide_recurso->recursos_pedidos, id_recurso);
    pthread_mutex_unlock(&m_listaEjecutando);
}

void sumar_instancia(int id_recurso, contexto_ejecucion* contexto) // tednria que haber un sumar_instancia por exit que saque los elementos de el pcb q esta de salida
{ // suma 1 a la instancia y se la saco a recursos_pedidos del proceso en ejecucion
    kernel_config.instancias_recursos[id_recurso] += 1;

    sem_post(sem_recurso[id_recurso]);

    t_pcb * pcb_quita_recurso = pcb_lget_devuelve_pcb(contexto, listaEjecutando, m_listaEjecutando);
    
    pthread_mutex_lock(&m_listaEjecutando);
        list_remove_element(pcb_quita_recurso->recursos_pedidos, id_recurso);
    pthread_mutex_unlock(&m_listaEjecutando);
}

void sumar_instancia_exit(int id_recurso, t_pcb* pcb_quita_recurso)
{
    kernel_config.instancias_recursos[id_recurso] += 1;

    sem_post(sem_recurso[id_recurso]);

    list_remove_element(pcb_quita_recurso->recursos_pedidos, id_recurso);
}

// ----------------------- Funciones WAIT_RECURSO ----------------------- //

void atender_wait_recurso()
{
    t_ce_string* estructura_wait_recurso = recibir_ce_string(cpu_dispatch_connection);

    contexto_ejecucion* contexto_ejecuta_wait = estructura_wait_recurso->ce;

    char* recurso_wait = estructura_wait_recurso->string;

    if (recurso_no_existe(recurso_wait)) {
        finalizar_proceso(contexto_ejecuta_wait, INVALID_RESOURCE);
    } else {
        int id_recurso = obtener_id_recurso(recurso_wait);

        restar_instancia(id_recurso, contexto_ejecuta_wait);
        
        log_info(kernel_logger, "PID: [%d] - Wait: [%s] - Instancias: [%d]", contexto_ejecuta_wait->id, recurso_wait, obtener_instancias_recurso(id_recurso));
        
        if (tiene_instancia_wait(id_recurso)) {
            t_pcb* pcb_ejecuta_wait = actualizar_pcb_lget_devuelve_pcb(contexto_ejecuta_wait, listaEjecutando, m_listaEjecutando);

            contexto_ejecucion* nuevo_contexto_ejecuta_wait = obtener_ce(pcb_ejecuta_wait);

            enviar_ce(cpu_dispatch_connection, nuevo_contexto_ejecuta_wait, EJECUTAR_CE, kernel_logger);

            liberar_ce(nuevo_contexto_ejecuta_wait);

            sem_wait(sem_recurso[id_recurso]);
        } else {

            t_pcb * pcb_bloqueado_en_recurso = actualizar_pcb_lremove_devuelve_pcb(contexto_ejecuta_wait, listaEjecutando, m_listaEjecutando);
            
            // pthread_mutex_lock(&m_listaEjecutando);
            //     t_pcb * pcb_bloqueado_en_recurso = (t_pcb *) list_remove(listaEjecutando, 0); // inicializar pcb y despues liberarlo
            //     actualizar_pcb(pcb_bloqueado_en_recurso, contexto_ejecuta_wait);
            // pthread_mutex_unlock(&m_listaEjecutando);
            
            cambiar_estado_a(pcb_bloqueado_en_recurso, BLOCKED, estadoActual(pcb_bloqueado_en_recurso));
            
            log_info(kernel_logger, "PID: [%d] - Bloqueado por: [%s]",pcb_bloqueado_en_recurso->id, recurso_wait);
            
            sacar_rafaga_ejecutada(pcb_bloqueado_en_recurso); // hacer cada vez que sale de running
            
            sem_post(&fin_ejecucion);
            
            bloqueo_proceso_en_recurso(pcb_bloqueado_en_recurso, id_recurso); // aca tengo que encolar en la lista correspondiente
        }
    }
    liberar_ce_string(estructura_wait_recurso);
}

int tiene_instancia_wait(int id_recurso)
{ // devuelve 1 si instancia recurso es >= 0, 0 en otro caso
    return (kernel_config.instancias_recursos[id_recurso] >= 0);
}

void bloqueo_proceso_en_recurso(t_pcb* pcb, int id_recurso)
{
    pthread_mutex_lock(m_listaRecurso[id_recurso]);
        list_add(lista_recurso[id_recurso], pcb);
    pthread_mutex_unlock(m_listaRecurso[id_recurso]);
}

// ----------------------- Funciones SIGNAL_RECURSO ----------------------- //

void atender_signal_recurso()
{
    t_ce_string* estructura_signal_recurso = recibir_ce_string(cpu_dispatch_connection);

    contexto_ejecucion* contexto_ejecuta_signal = estructura_signal_recurso->ce;

    char* recurso_signal = estructura_signal_recurso->string;

    if (recurso_no_existe(recurso_signal)) {
        finalizar_proceso(contexto_ejecuta_signal, INVALID_RESOURCE);
    } else {
        t_pcb * pcb_ejecuta_signal = actualizar_pcb_lget_devuelve_pcb(contexto_ejecuta_signal, listaEjecutando, m_listaEjecutando);

        contexto_ejecucion* nuevo_contexto_ejecuta_signal = obtener_ce(pcb_ejecuta_signal);
        
        enviar_ce(cpu_dispatch_connection, nuevo_contexto_ejecuta_signal, EJECUTAR_CE, kernel_logger);

        liberar_ce(nuevo_contexto_ejecuta_signal);

        int id_recurso = obtener_id_recurso(recurso_signal);

        sumar_instancia(id_recurso, nuevo_contexto_ejecuta_signal);
        
        log_info(kernel_logger, "PID: [%d] - Signal: [%s] - Instancias: [%d]", nuevo_contexto_ejecuta_signal->id, recurso_signal, obtener_instancias_recurso(id_recurso));
                            
        if (tiene_que_reencolar_bloq_recurso(id_recurso)) {
            reencolar_bloqueo_por_recurso(id_recurso);
        }
    }
    liberar_ce_string(estructura_signal_recurso);
}

int tiene_que_reencolar_bloq_recurso(int id_recurso)
{
    return (kernel_config.instancias_recursos[id_recurso] <= 0);
}

void reencolar_bloqueo_por_recurso(int id_recurso)
{
    sem_wait(sem_recurso[id_recurso]);

    pthread_mutex_lock(m_listaRecurso[id_recurso]);
        t_pcb * pcb_a_reencolar = (t_pcb *) list_remove(lista_recurso[id_recurso], 0); // inicializar pcb y despues liberarlo
    pthread_mutex_unlock(m_listaRecurso[id_recurso]);
    
    cambiar_estado_a(pcb_a_reencolar, READY, estadoActual(pcb_a_reencolar));
    
    iniciar_nueva_espera_ready(pcb_a_reencolar); // hacer cada vez que se mete en la lista de ready
    
    agregar_a_lista_con_sems(pcb_a_reencolar, listaReady, m_listaReady);
    
    sem_post(&proceso_en_ready);
}

// ----------------------- Funciones BLOCK_IO ----------------------- //

void atender_block_io()
{
    t_ce_string* estructura_block_io = recibir_ce_string(cpu_dispatch_connection);

    char* tiempo_bloqueo = estructura_block_io->string;
    
    contexto_ejecucion* contexto_IO = estructura_block_io->ce;
    
    int bloqueo = atoi(tiempo_bloqueo);
    
    t_pcb * pcb_IO = actualizar_pcb_lremove_devuelve_pcb(contexto_IO, listaEjecutando, m_listaEjecutando);
    
    // pthread_mutex_lock(&m_listaEjecutando);
    //     t_pcb * pcb_IO = (t_pcb *) list_remove(listaEjecutando, 0);
    //     actualizar_pcb(pcb_IO, contexto_IO);
    // pthread_mutex_unlock(&m_listaEjecutando);
    
    sacar_rafaga_ejecutada(pcb_IO); // hacer cada vez que sale de running
    
    sem_post(&fin_ejecucion);
    
    cambiar_estado_a(pcb_IO, BLOCKED, estadoActual(pcb_IO));

    log_info(kernel_logger, "PID: [%d] - Bloqueado por: [IO]",pcb_IO->id);
    
    agregar_a_lista_con_sems(pcb_IO, listaBloqueados, m_listaBloqueados);

    thread_args* argumentos = malloc(sizeof(thread_args));
    argumentos->pcb = pcb_IO;
    argumentos->bloqueo = bloqueo;

    //pthread_mutex_lock(&m_IO);
    pthread_create(&hiloIO, NULL, (void*) rutina_io, (void*) (thread_args*) argumentos);
    pthread_detach(hiloIO);

    
    liberar_ce_string(estructura_block_io);
}

void rutina_io(thread_args* args)
{
    t_pcb* pcb = args->pcb;
    uint32_t bloqueo = args->bloqueo;

    log_info(kernel_logger,"PID: [%d] - Ejecuta IO: [%d]",pcb->id, bloqueo);

    sleep(bloqueo);
    
    pthread_mutex_lock(&m_listaBloqueados);
        list_remove_element(listaBloqueados, pcb);
    pthread_mutex_unlock(&m_listaBloqueados);

    cambiar_estado_a(pcb, READY, estadoActual(pcb));
    
    iniciar_nueva_espera_ready(pcb); // hacer cada vez que se mete en la lista de ready
    
    agregar_a_lista_con_sems(pcb, listaReady, m_listaReady);
    
    sem_post(&proceso_en_ready);

    //pthread_mutex_unlock(&m_IO);
}

// ----------------------- Funciones CREAR_SEGMENTO ----------------------- //

void atender_crear_segmento()
{
    t_ce_2enteros* estructura_crear_segmento = recibir_ce_2enteros(cpu_dispatch_connection);

    contexto_ejecucion* contexto_crea_segmento = estructura_crear_segmento->ce;

    uint32_t id_segmento = estructura_crear_segmento->entero1;

    uint32_t tamanio_segmento = estructura_crear_segmento->entero2;
    
    uint32_t id_proceso = contexto_crea_segmento->id;

    log_info(kernel_logger, "PID: [%d] - Crear Segmento - Id: [%d] - Tamaño: [%d]", id_proceso, id_segmento, tamanio_segmento);

    enviar_3_enteros(memory_connection, id_proceso, id_segmento, tamanio_segmento, CREATE_SEGMENT); // mando a memoria idP, idS, tamanio

    uint32_t compactacion = 1;
    while(compactacion){
        int cod_op_creacion = recibir_operacion(memory_connection);
        switch (cod_op_creacion)
        {
        case MENSAJE:
            log_error(kernel_logger, "El codop dio mensaje");
        break;
        case OUT_OF_MEMORY:
            finalizar_proceso(contexto_crea_segmento, OUT_OF_MEMORY);
            compactacion = 0;
            break;
        
        case NECESITO_COMPACTAR:

            if (f_execute) { // si se ejecuta una instruccion f_* bloqueante
                log_info(kernel_logger, "Compactacion: [Esperando Fin de Operaciones de FS]");
                pthread_mutex_lock(&m_F_operation);
                pthread_mutex_unlock(&m_F_operation); // esto me parece una warangada, pero creo que funciona
            }

            atender_compactacion(id_proceso, id_segmento, tamanio_segmento);
            break;

        case OK:
        
            uint32_t base_segmento = recibir_entero_u32(memory_connection, kernel_logger);
            
            t_ent_ts *nuevoElemento = crear_segmento(id_segmento, base_segmento, tamanio_segmento);
            
            t_pcb* pcb_crea_segmento = actualizar_pcb_lget_devuelve_pcb(contexto_crea_segmento, listaEjecutando, m_listaEjecutando);
            
            list_replace_and_destroy_element(pcb_crea_segmento->tabla_segmentos, id_segmento, nuevoElemento, (void*)free);

            contexto_ejecucion* nuevo_contexto_crea_segmento = obtener_ce(pcb_crea_segmento);

            enviar_ce(cpu_dispatch_connection, nuevo_contexto_crea_segmento, EJECUTAR_CE, kernel_logger);
            log_warning(kernel_logger, "Aca estoy en OK x3");
            liberar_ce(nuevo_contexto_crea_segmento);

            compactacion = 0;
            break;
        default:
            log_error(kernel_logger, "El codigo de recepcion de la creacion del segmento es erroneo");
            break;
        }
    }
    
    liberar_ce_2enteros(estructura_crear_segmento); // VER SI FUNCIONA
}

void atender_compactacion(uint32_t id_proceso, uint32_t id_segmento, uint32_t tamanio_segmento)
{
    log_info(kernel_logger, "Compactacion: [Se Solicito Compactacion]");

    enviar_CodOp(memory_connection, COMPACTAR);

    uint32_t cod_op_compactacion = recibir_operacion(memory_connection);
    log_error(kernel_logger,"el codop es %d",cod_op_compactacion);
    
    switch (cod_op_compactacion){
        case OK_COMPACTACION:
            
            recibir_nuevas_bases();
            // actualizar_ts_x_proceso();

            log_info(kernel_logger, "Compactacion: [Finalizo el proceso de compactacion]");

            enviar_3_enteros(memory_connection, id_proceso, id_segmento, tamanio_segmento, CREATE_SEGMENT);

            break;
        
        default:
            log_error(kernel_logger, "El codigo de recepcion de la compactacion del segmento es erroneo");

            break;
    }
}

void recibir_nuevas_bases(int socket_memoria) {
    uint32_t tam_buffer;
    recv(memory_connection, &tam_buffer, sizeof(uint32_t), NULL);
    tam_buffer -= sizeof(uint32_t);

    void *buffer = malloc(tam_buffer);
    recv(memory_connection, buffer, tam_buffer, NULL);
    int desplazamiento = 0;

    int cant_segmentos = tam_buffer / (sizeof(uint32_t) * 3);
    
    uint32_t pid;
    uint32_t id;
    uint32_t n_base;
    t_list* lista_de_pcbs = list_create();

    list_add_all(lista_de_pcbs, listaReady);
    list_add_all(lista_de_pcbs, listaEjecutando);
    list_add_all(lista_de_pcbs, listaBloqueados);

    for(int i = 0; i < cantidad_instancias; i++)
    {
        list_add_all(lista_de_pcbs, lista_recurso[i]);
    }

    for (int i = 0; i < cant_segmentos; i++)
    {
        memcpy(&pid, buffer + desplazamiento, sizeof(uint32_t));
        desplazamiento += sizeof(uint32_t);
        memcpy(&id, buffer + desplazamiento, sizeof(uint32_t));
        desplazamiento += sizeof(uint32_t);
        memcpy(&n_base, buffer + desplazamiento, sizeof(uint32_t));
        desplazamiento += sizeof(uint32_t);


        t_pcb* pcb_encontrado = pcb_en_lista_coincide(lista_de_pcbs, pid);

        // imprimir_tabla_segmentos(pcb_encontrado->tabla_segmentos,kernel_logger);
        // Search PID in PROCESOS_EN_MEMORIA
        
        t_ent_ts* segmento = list_get(pcb_encontrado->tabla_segmentos, id);

        segmento->base = n_base;
        
    }
    free(buffer);
}

t_pcb* pcb_en_lista_coincide(t_list* lista_pcbs, uint32_t idproceso)
{
    bool encontrar_pcb(t_pcb* pcb){
        return pcb->id == idproceso;
    }

    return list_find(lista_pcbs, (void*) encontrar_pcb);
}

// ----------------------- Funciones BORRAR_SEGMENTO ----------------------- //

void atender_borrar_segmento() //TODO
{
    t_ce_entero* estructura_borrar_segmento = recibir_ce_entero(cpu_dispatch_connection);

    contexto_ejecucion* contexto_borrar_segmento = estructura_borrar_segmento->ce;

    t_pcb* pcb_a_borrar = pcb_en_lista_coincide(listaEjecutando, contexto_borrar_segmento->id);

    uint32_t id_proceso = pcb_a_borrar->id;

    uint32_t id_segmento = estructura_borrar_segmento->entero;
    
    t_ent_ts* seg_a_modificar = list_get(pcb_a_borrar->tabla_segmentos, id_segmento);

    uint32_t base_segmento = seg_a_modificar->base;

    uint32_t tam_segmento = seg_a_modificar->tam;

    solicitar_liberacion_segmento(base_segmento, tam_segmento, id_proceso, id_segmento);

    seg_a_modificar->base = 0;
    seg_a_modificar->tam = 0;
    seg_a_modificar->activo = 0;
    
    log_info(kernel_logger, "PID: [%d] - Eliminar Segmento - Id Segmento: [%d]", id_proceso, id_segmento);
    
    t_pcb* pcb_borrar_segmento = actualizar_pcb_lget_devuelve_pcb(contexto_borrar_segmento, listaEjecutando, m_listaEjecutando);

    // imprimir_tabla_segmentos(pcb_borrar_segmento->tabla_segmentos, kernel_logger);

    contexto_ejecucion* nuevo_contexto_borrar_segmento = obtener_ce(pcb_borrar_segmento);
    
    enviar_ce(cpu_dispatch_connection, nuevo_contexto_borrar_segmento, EJECUTAR_CE, kernel_logger);
    
    liberar_ce(nuevo_contexto_borrar_segmento);

    liberar_ce_entero(estructura_borrar_segmento);
}

void limpiar_tabla_segmentos(t_list* tabla_segmentos)
{
    list_clean_and_destroy_elements(tabla_segmentos, (void*) free);
}

int obtener_tamanio_segmento(t_pcb* pcb,int id_segmento_elim){

    t_segmento* segmento_a_eliminar = list_find(pcb->tabla_segmentos, id_segmento_elim);
    return segmento_a_eliminar->tamanio_segmento;

}
int obtener_base_segmento(t_pcb* pcb,int id_segmento_elim){

    t_segmento* segmento_a_eliminar = list_find(pcb->tabla_segmentos, id_segmento_elim);
    return segmento_a_eliminar->direccion_base;

}
// ----------------------- Funciones ABRIR_ARCHIVO ----------------------- //
void atender_apertura_archivo()
{
    t_ce_string* estructuraApertura = recibir_ce_string(cpu_dispatch_connection);

    contexto_ejecucion* contextoDeEjecucion = estructuraApertura->ce;

    char* nombreArchivo = estructuraApertura->string;

    t_list* nombreArchivosAbiertos = list_map(tablaGlobalArchivosAbiertos,(void*) obtener_nombre_archivo);

    t_pcb * pcb_en_ejecucion = actualizar_pcb_lget_devuelve_pcb(contextoDeEjecucion, listaEjecutando, m_listaEjecutando);
    
    // pthread_mutex_lock(&m_listaEjecutando);
    //     t_pcb * pcb_en_ejecucion = (t_pcb *) list_get(listaEjecutando, 0);
    //     actualizar_pcb(pcb_en_ejecucion, contextoDeEjecucion);
    // pthread_mutex_unlock(&m_listaEjecutando);
    
    if(existeArchivo(nombreArchivo)){ // si existe en la tabla global de kernel
        log_warning(kernel_logger, "Kernel dice que existe el archivo en la tabla global, does kernel miente?");
        t_entradaTAAP* entradaTAAP3= malloc(sizeof(t_entradaTAAP));
        crear_entrada_TAAP(nombreArchivo,entradaTAAP3);
        list_add(pcb_en_ejecucion->tabla_archivos_abiertos_por_proceso,entradaTAAP3);
        t_entradaTGAA* entradaEncontrada = obtenerEntrada(nombreArchivo);

        sacar_rafaga_ejecutada(pcb_en_ejecucion);
        cambiar_estado_a(pcb_en_ejecucion, BLOCKED, estadoActual(pcb_en_ejecucion));
        sem_post(&fin_ejecucion);
        agregar_a_lista_con_sems(pcb_en_ejecucion, entradaEncontrada->lista_block_archivo, entradaEncontrada->m_lista_block_archivo);
        log_info(kernel_logger, "PID: [%d] - Bloqueado por: [%s]", pcb_en_ejecucion->id, nombreArchivo);

    }
    else{
        enviar_paquete_string(file_system_connection,nombreArchivo,F_OPEN,(strlen(nombreArchivo)+1));
        
        int existe = recibir_operacion(file_system_connection);

        switch(existe){
            case NO_EXISTE_ARCHIVO:

                t_entradaTAAP* entradaTAAP = malloc(sizeof(t_entradaTAAP));
                log_trace(kernel_logger,"recibimos no existe");
                enviar_paquete_string(file_system_connection,nombreArchivo,F_CREATE,(strlen(nombreArchivo)+1));
                log_trace(kernel_logger,"enviamos el f_CREATE");
                crear_entrada_TGAA(nombreArchivo,entradaTAAP);
                log_trace(kernel_logger,"Creamos Entrada TGAA");
                crear_entrada_TAAP(nombreArchivo,entradaTAAP);
                list_add(pcb_en_ejecucion->tabla_archivos_abiertos_por_proceso,entradaTAAP);
                contexto_ejecucion* contextoAEnviar = obtener_ce(pcb_en_ejecucion);
                enviar_ce(cpu_dispatch_connection,contextoAEnviar,EJECUTAR_CE,kernel_logger);
                log_trace(kernel_logger,"llegamos al final del NO_EXISTE_ARCHIVO");
            
                break;

            case EXISTE_ARCHIVO: // existe en el FS
                log_error(kernel_logger, "El abrir archivo entro por aca");
                t_entradaTAAP* entradaTAAP2 = malloc(sizeof(t_entradaTAAP));

                crear_entrada_TGAA(nombreArchivo, entradaTAAP2);

                crear_entrada_TAAP(nombreArchivo, entradaTAAP2);

                list_add(pcb_en_ejecucion->tabla_archivos_abiertos_por_proceso, entradaTAAP2);

                contexto_ejecucion* contextoAEnviar2 = obtener_ce(pcb_en_ejecucion);

                enviar_ce(cpu_dispatch_connection, contextoAEnviar2, EJECUTAR_CE, kernel_logger);
                
                break;

            default:
                log_error(kernel_logger,"CodOp invalido");
                break;
            
        }
        log_info(kernel_logger, "PID: [%d] - Abrir Archivo: [%s]",pcb_en_ejecucion->id, nombreArchivo);
    }
    liberar_ce_string_entero(estructuraApertura);
}

void crear_entrada_TGAA(char* nombre, t_entradaTAAP* entrada)
{
    t_entradaTGAA* nuevaEntradaTGAA = malloc(sizeof(t_entradaTGAA));

    strcpy(nuevaEntradaTGAA->nombreArchivo,nombre);

    nuevaEntradaTGAA->tamanioArchivo = 0;
    
    nuevaEntradaTGAA->lista_block_archivo = list_create();

    pthread_mutex_init(&(nuevaEntradaTGAA->m_lista_block_archivo),NULL);

    list_add(tablaGlobalArchivosAbiertos, nuevaEntradaTGAA);
}

void crear_entrada_TAAP(char* nombre,t_entradaTAAP* nuevaEntrada){

    strcpy(nuevaEntrada->nombreArchivo,nombre);  

    nuevaEntrada->puntero = 0;

    log_trace(kernel_logger, "Tamaño tabla global:%d ", list_size(tablaGlobalArchivosAbiertos));

    t_list* listaFiltrada = nombre_en_lista_coincide(tablaGlobalArchivosAbiertos, nombre);

    log_trace(kernel_logger, "Logre filtrar la lista por nombre con tamaño:%d ", list_size(listaFiltrada));
    
    t_entradaTGAA* entradaGlobal = list_get(listaFiltrada,0);
    
    log_warning(kernel_logger,"el elemento guardado en TAAP tiene:%s,  tamanio %d",entradaGlobal->nombreArchivo,entradaGlobal->tamanioArchivo);
    
    nuevaEntrada->tamanioArchivo = entradaGlobal->tamanioArchivo;

    list_destroy(listaFiltrada);
}

t_list* nombre_en_lista_coincide(t_list* tabla, char* nombre)
{
    bool encontrar_nombre(t_entradaTGAA* entrada){
    
        log_trace(kernel_logger, "Busco en lista de entradas, nombre en lista coincide, %s", entrada->nombreArchivo);
        return strcmp(entrada->nombreArchivo, nombre) == 0;
    }

    return list_filter(tabla, (void*)encontrar_nombre);
}

t_list* nombre_en_lista_coincide_TAAP(t_list* tabla, char* nombre)
{
    bool encontrar_nombre(t_entradaTAAP* entrada){
    
        log_trace(kernel_logger, "Busco en lista de entradas, TAAP, %s", entrada->nombreArchivo);
        return strcmp(entrada->nombreArchivo, nombre) == 0;
    }

    return list_filter(tabla, (void*)encontrar_nombre);
}

t_list* nombre_en_lista_nombres_coincide(t_list* tabla, char* nombre)
{
    bool encontrar_nombre(char* entrada){
        log_trace(kernel_logger, "Busco en lista de nombres");
        return strcmp(entrada, nombre) == 0;
        }

    return list_filter(tabla, (void*)encontrar_nombre);
}
// ----------------------- Funciones CERRAR_ARCHIVO ----------------------- //
void atender_cierre_archivo(){

    t_ce_string* estructuraCierre=recibir_ce_string(cpu_dispatch_connection);
    contexto_ejecucion* contextoDeEjecucion = estructuraCierre->ce;
    char* nombreArchivo = estructuraCierre->string;

    t_pcb * pcb_en_ejecucion = actualizar_pcb_lget_devuelve_pcb(contextoDeEjecucion, listaEjecutando, m_listaEjecutando);

    // pthread_mutex_lock(&m_listaEjecutando);
    //     t_pcb * pcb_en_ejecucion = (t_pcb *) list_get(listaEjecutando, 0);
    //     actualizar_pcb(pcb_en_ejecucion, contextoDeEjecucion);
    // pthread_mutex_unlock(&m_listaEjecutando);

    log_info(kernel_logger, "PID: [%d] - Cerrar Archivo: [%s]",pcb_en_ejecucion->id,nombreArchivo);
    
    t_list* listaFiltrada = nombre_en_lista_coincide(pcb_en_ejecucion->tabla_archivos_abiertos_por_proceso,(char*) nombreArchivo);
    
    log_error(kernel_logger,"la lista tiene :%d elementos", list_size(listaFiltrada));

    t_entradaTAAP* entradaProceso = list_get(listaFiltrada,0);
    
    t_entradaTGAA* entradaGlobalAA = conseguirEntradaTablaGlobal(nombreArchivo);

    list_remove_element(entradaGlobalAA->lista_block_archivo, pcb_en_ejecucion);

    log_error(kernel_logger,"la TAAP tiene :%d elementos", list_size(pcb_en_ejecucion->tabla_archivos_abiertos_por_proceso));

    list_remove_element(pcb_en_ejecucion->tabla_archivos_abiertos_por_proceso, entradaProceso);

    log_error(kernel_logger,"la TAAP tiene :%d elementos", list_size(pcb_en_ejecucion->tabla_archivos_abiertos_por_proceso));
    
    free(entradaProceso);
    
    if(otrosUsanArchivo(nombreArchivo)){
        log_debug(kernel_logger, "Entro al log que dice que hay otros usuarios abriendo el archivo");
        t_pcb* pcb_a_ejectuar = hallarPrimerPcb(nombreArchivo);
        enviar_ce(cpu_dispatch_connection, contextoDeEjecucion, EJECUTAR_CE, kernel_logger);
        reencolar_bloq_por_archivo(nombreArchivo,entradaGlobalAA);
    }
    
    else{
        log_debug(kernel_logger, "no hay mas usuarios abriendo el archivo, mamawebo");
        t_list* listaFiltradaEnElse =  nombre_en_lista_coincide(tablaGlobalArchivosAbiertos,(char*) nombreArchivo);
        t_entradaTGAA* entradaGlobal = list_get(listaFiltradaEnElse,0);
        list_remove_element(tablaGlobalArchivosAbiertos,entradaGlobal);
        log_warning(kernel_logger,"la TAAG tiene :%d",list_size(tablaGlobalArchivosAbiertos));
        enviar_ce(cpu_dispatch_connection, contextoDeEjecucion, EJECUTAR_CE, kernel_logger);
        list_destroy(listaFiltradaEnElse);
        free(entradaGlobal);
    }

    list_destroy(listaFiltrada);
    liberar_ce_string_entero(estructuraCierre);
}

void reencolar_bloq_por_archivo(char* nombreArchivo,t_entradaTGAA* entrada){
    pthread_mutex_lock(&entrada->m_lista_block_archivo);
        t_pcb * pcb_a_reencolar = hallarPrimerPcb(nombreArchivo);
    pthread_mutex_unlock(&entrada->m_lista_block_archivo);
    
    cambiar_estado_a(pcb_a_reencolar, READY, estadoActual(pcb_a_reencolar));
    
    iniciar_nueva_espera_ready(pcb_a_reencolar); // hacer cada vez que se mete en la lista de ready
    
    agregar_a_lista_con_sems(pcb_a_reencolar, listaReady, m_listaReady);
    
    sem_post(&proceso_en_ready);
}
/*
void reencolar_bloqueo_por_recurso(int id_recurso)
{
    sem_wait(sem_recurso[id_recurso]);

    pthread_mutex_lock(m_listaRecurso[id_recurso]);
        t_pcb * pcb_a_reencolar = (t_pcb *) list_remove(lista_recurso[id_recurso], 0); // inicializar pcb y despues liberarlo
    pthread_mutex_unlock(m_listaRecurso[id_recurso]);
    
    cambiar_estado_a(pcb_a_reencolar, READY, estadoActual(pcb_a_reencolar));
    
    iniciar_nueva_espera_ready(pcb_a_reencolar); // hacer cada vez que se mete en la lista de ready
    
    agregar_a_lista_con_sems(pcb_a_reencolar, listaReady, m_listaReady);
    
    sem_post(&proceso_en_ready);*/

t_entradaTGAA* conseguirEntradaTablaGlobal(char* nombreArchivo){
    t_list* listaFiltrada = nombre_en_lista_coincide(tablaGlobalArchivosAbiertos,(char*)nombreArchivo);
    t_entradaTGAA* entradaConseguida = list_get(listaFiltrada, 0);
    list_destroy(listaFiltrada);
    return entradaConseguida;
}

t_pcb* hallarPrimerPcb(char* nombreArchivo){
    t_entradaTGAA* entradaGlobal = conseguirEntradaTablaGlobal(nombreArchivo);

return list_get(entradaGlobal->lista_block_archivo,0);
}

bool otrosUsanArchivo(char* nombreArchivo) {
t_entradaTGAA* entradaGlobal = conseguirEntradaTablaGlobal(nombreArchivo);

return list_is_empty(entradaGlobal->lista_block_archivo) == 0;
}




// ----------------------- Funciones ACTUALIZAR_PUNTERO ----------------------- //
void atender_actualizar_puntero(){
    log_error(kernel_logger,"entro");
    t_ce_string_entero* estructura_actualizacion = malloc(sizeof(t_ce_string_entero));
    
    estructura_actualizacion = recibir_ce_string_entero(cpu_dispatch_connection);

    uint32_t posicion = estructura_actualizacion->entero;
    char* nombreArchivo = estructura_actualizacion->string;
         
    log_error(kernel_logger,"el nombre del archivo es %s",nombreArchivo);
    log_error(kernel_logger,"el entero del paquete es %d",posicion);
    // imprimir_ce(estructura_actualizacion->ce, kernel_logger);
    contexto_ejecucion* contextoDeEjecucion=estructura_actualizacion->ce;

    t_pcb * pcb_en_ejecucion = actualizar_pcb_lget_devuelve_pcb(contextoDeEjecucion, listaEjecutando, m_listaEjecutando);

    // pthread_mutex_lock(&m_listaEjecutando);
    // t_pcb * pcb_en_ejecucion = (t_pcb *) list_get(listaEjecutando, 0);
    // actualizar_pcb(pcb_en_ejecucion, contextoDeEjecucion);
    // pthread_mutex_unlock(&m_listaEjecutando);

    log_error(kernel_logger,"estoy por filtrar lista");
    
    t_list* listaFiltrada = nombre_en_lista_coincide(pcb_en_ejecucion->tabla_archivos_abiertos_por_proceso,nombreArchivo);
    
    log_error(kernel_logger,"el tamanio de la lista es %d",list_size(listaFiltrada));

    t_entradaTAAP* entradaProceso = list_get(listaFiltrada,0);

    log_error(kernel_logger,"consegui posicion inicial %u",entradaProceso->puntero);
    log_error(kernel_logger,"consegui entrada %s",entradaProceso->nombreArchivo);
    log_error(kernel_logger,"consegui posicion 2da %d",posicion);
    
    entradaProceso->puntero = (uint32_t) posicion;

    log_error(kernel_logger,"consegui posicion %u",entradaProceso->puntero);
    
    contexto_ejecucion* contextoAEnviar = obtener_ce(pcb_en_ejecucion);
    enviar_ce(cpu_dispatch_connection,contextoAEnviar,EJECUTAR_CE,kernel_logger);
    //falta liberar algo?
    log_info(kernel_logger, "PID: [%d] - Actualizar puntero Archivo: [%s] - Puntero [%u]",pcb_en_ejecucion->id,entradaProceso->nombreArchivo,entradaProceso->puntero);

    list_destroy(listaFiltrada);
    liberar_ce_string_entero(estructura_actualizacion);
    
}
// ----------------------- Funciones LEER_ARCHIVO ----------------------- //
void atender_lectura_archivo(){

    bloquear_FS();
    log_error(kernel_logger,"por entrar a recibir ce string 3 enteros");
    t_ce_string_3enteros* estructura_leer_archivo= recibir_ce_string_3enteros(cpu_dispatch_connection);
    log_error(kernel_logger,"el nombre es :%s",estructura_leer_archivo->string);
    contexto_ejecucion* ce_a_updatear = estructura_leer_archivo->ce;

    t_pcb * pcb_lectura = actualizar_pcb_lget_devuelve_pcb(ce_a_updatear, listaEjecutando, m_listaEjecutando);

    char nombre_archivo[100];
    strcpy(nombre_archivo,estructura_leer_archivo->string);

    uint32_t dir_fisica = estructura_leer_archivo ->entero1;

    uint32_t bytes_a_leer = estructura_leer_archivo->entero2;

    uint32_t estavariablenosirve = estructura_leer_archivo->entero3;

    t_list* lista_filtrada = nombre_en_lista_coincide_TAAP(pcb_lectura->tabla_archivos_abiertos_por_proceso, estructura_leer_archivo->string);
    log_debug(kernel_logger, "La lista filtrada tiene %d cantidad de elementos", list_size(lista_filtrada));
    t_entradaTAAP* entradaProceso = list_get(lista_filtrada,0);
    uint32_t puntero_archivo = entradaProceso->puntero;

    log_info(kernel_logger, "PID: [%d] - Leer Archivo: [%s] - Puntero [%d] - Dirección Memoria [%d] - Tamaño [%d]", ce_a_updatear->id, nombre_archivo, puntero_archivo, dir_fisica, bytes_a_leer); // verificar con el resto

    
    // pthread_mutex_lock(&m_listaEjecutando);
    //     t_pcb * pcb_lectura = (t_pcb *) list_get(listaEjecutando, 0);
    //     actualizar_pcb(pcb_lectura, ce_a_updatear);
    // pthread_mutex_unlock(&m_listaEjecutando);
    
    sacar_rafaga_ejecutada(pcb_lectura); // hacer cada vez que sale de running
    
    cambiar_estado_a(pcb_lectura, BLOCKED, estadoActual(pcb_lectura));

    log_info(kernel_logger, "PID: [%d] - Bloqueado por: [%s]",pcb_lectura->id, nombre_archivo);
    
    agregar_a_lista_con_sems(pcb_lectura, listaBloqueados, m_listaBloqueados);

    thread_args_read* argumentos = malloc(sizeof(thread_args_read));
    argumentos->pcb = pcb_lectura;
    strcpy(argumentos->nombre,nombre_archivo);
    argumentos->puntero = puntero_archivo;
    argumentos->bytes = bytes_a_leer;
    argumentos->dir_fisica = dir_fisica;

    pthread_create(&hiloRead, NULL, (void*) rutina_read, (void*) (thread_args_read*) argumentos);
    pthread_detach(hiloRead);

    sem_post(&fin_ejecucion);

    list_destroy(lista_filtrada);
    liberar_ce_string_2enteros(estructura_leer_archivo);

}
   
void rutina_read(thread_args_read* args)
{
    sem_post(&fin_ejecucion);
    t_pcb* pcb = args->pcb;
    char nombre [100];
    strcpy(nombre,args->nombre);
    uint32_t puntero = args->puntero;
    uint32_t bytes = args->bytes;
    uint32_t dir_fisica = args->dir_fisica;

    enviar_string_4enteros(file_system_connection, nombre, pcb->id, puntero,bytes, dir_fisica, F_READ);
    log_error(kernel_logger,"Envio fread a fs");

    uint32_t cod_op = recibir_operacion(file_system_connection);

    log_warning(kernel_logger, "El codigo de operacion que recibo en la rutina read es: %d", cod_op);

    t_pcb* pcb_activo = pcb_lremove(obtener_ce(pcb), listaBloqueados, m_listaBloqueados);

    cambiar_estado_a(pcb_activo, READY, estadoActual(pcb_activo));
    
    iniciar_nueva_espera_ready(pcb_activo); // hacer cada vez que se mete en la lista de ready
    
    agregar_a_lista_con_sems(pcb_activo, listaReady, m_listaReady);
    
    sem_post(&proceso_en_ready);
    desbloquear_FS();
}

// ----------------------- Funciones ESCRIBIR_ARCHIVO ----------------------- //
void atender_escritura_archivo(){

    bloquear_FS();

    t_ce_string_3enteros* estructura_escribir_archivo= recibir_ce_string_3enteros(cpu_dispatch_connection);

    contexto_ejecucion* ce_a_updatear = estructura_escribir_archivo->ce;

    t_pcb * pcb_escritura = actualizar_pcb_lget_devuelve_pcb(ce_a_updatear, listaEjecutando, m_listaEjecutando);
    
    char nombre_archivo[100];
    strcpy(nombre_archivo,estructura_escribir_archivo->string);

    uint32_t direccion_fisica = estructura_escribir_archivo->entero1;
    uint32_t bytes_a_leer = estructura_escribir_archivo->entero2;
    uint32_t offset = estructura_escribir_archivo->entero3;
    t_list* lista_filtrada = nombre_en_lista_coincide_TAAP(pcb_escritura->tabla_archivos_abiertos_por_proceso, estructura_escribir_archivo->string);
    t_entradaTAAP* entradaProceso = list_get(lista_filtrada,0);
    uint32_t puntero_archivo = entradaProceso->puntero;

    log_info(kernel_logger, "PID: [%d] - Escribir Archivo: [%s] - Puntero [%d] - Dirección Memoria [%d] - Tamaño [%d]", ce_a_updatear->id, nombre_archivo, puntero_archivo, offset, bytes_a_leer);
    

    // pthread_mutex_lock(&m_listaEjecutando);
    //     t_pcb * pcb_escritura = (t_pcb *) list_get(listaEjecutando, 0);
    //     actualizar_pcb(pcb_escritura, ce_a_updatear);
    // pthread_mutex_unlock(&m_listaEjecutando);

    sacar_rafaga_ejecutada(pcb_escritura); // hacer cada vez que sale de running
    
    cambiar_estado_a(pcb_escritura, BLOCKED, estadoActual(pcb_escritura));

    log_info(kernel_logger, "PID: [%d] - Bloqueado por: [%s]",pcb_escritura->id, nombre_archivo);
    
    agregar_a_lista_con_sems(pcb_escritura, listaBloqueados, m_listaBloqueados);

    thread_args_write* argumentos = malloc(sizeof(thread_args_write));
    argumentos->pcb = pcb_escritura;
    strcpy(argumentos->nombre,nombre_archivo);
    argumentos->puntero = puntero_archivo;
    argumentos->bytes = bytes_a_leer;
    argumentos->offset = offset;
    argumentos->dir_fisica = direccion_fisica;

    pthread_create(&hiloWrite, NULL, (void*) rutina_write, (void*) (thread_args_write*) argumentos);
    pthread_detach(hiloWrite);
    
    
    list_destroy(lista_filtrada);

    liberar_ce_string_2enteros(estructura_escribir_archivo);
}

void rutina_write(thread_args_write* args)
{
    sem_post(&fin_ejecucion);
    t_pcb* pcb = args->pcb;
    char nombre [100];
    strcpy(nombre,args->nombre);
    uint32_t puntero = args->puntero;
    uint32_t bytes = args->bytes;
    uint32_t offset = args->offset;
    uint32_t dir_fisica = args->dir_fisica;

      log_error(kernel_logger,"el nombre en rutina es :%s, el puntero es: %d",nombre, puntero);
    
    enviar_string_5enteros(file_system_connection, nombre, pcb->id, puntero,bytes,offset, dir_fisica, F_WRITE);
    
    uint32_t cod_op = recibir_operacion(file_system_connection);

    log_warning(kernel_logger,"recibi CODOP de fwrite %d",cod_op);

    pthread_mutex_lock(&m_listaBloqueados);
        list_remove_element(listaBloqueados, pcb);
    pthread_mutex_unlock(&m_listaBloqueados);

    cambiar_estado_a(pcb, READY, estadoActual(pcb));
    
    iniciar_nueva_espera_ready(pcb); // hacer cada vez que se mete en la lista de ready
    
    agregar_a_lista_con_sems(pcb, listaReady, m_listaReady);
    
    sem_post(&proceso_en_ready);
    desbloquear_FS();
}

void bloquear_FS(){
    f_execute = 1;
    pthread_mutex_lock(&m_F_operation);
}

void desbloquear_FS(){
    f_execute = 0;
    pthread_mutex_unlock(&m_F_operation);
}

// ----------------------- Funciones MODIFICAR_TAMANIO_ARCHIVO ----------------------- //
void atender_modificar_tamanio_archivo(){

    t_ce_string_entero* estructura_mod_tam_archivo = recibir_ce_string_entero(cpu_dispatch_connection);

    contexto_ejecucion* contexto_mod_tam_arch = estructura_mod_tam_archivo->ce;

    char nombre_archivo[100];
    strcpy(nombre_archivo,estructura_mod_tam_archivo->string);

    log_error(kernel_logger,"el nombre es :%s",nombre_archivo);
    uint32_t tamanio_archivo = estructura_mod_tam_archivo->entero;
    log_error(kernel_logger,"el tamanio es :%d",tamanio_archivo);
    
    log_info(kernel_logger, "PID: [%d] - Truncar Archivo: [%s] - Tamaño: [%d]", contexto_mod_tam_arch->id, nombre_archivo, tamanio_archivo);

    t_pcb * pcb_mod_tam_arch = actualizar_pcb_lget_devuelve_pcb(contexto_mod_tam_arch, listaEjecutando, m_listaEjecutando);
    
    // pthread_mutex_lock(&m_listaEjecutando);
    //     t_pcb * pcb_mod_tam_arch = (t_pcb *) list_get(listaEjecutando, 0);
    //     actualizar_pcb(pcb_mod_tam_arch, contexto_mod_tam_arch);
    // pthread_mutex_unlock(&m_listaEjecutando);

    sacar_rafaga_ejecutada(pcb_mod_tam_arch); // hacer cada vez que sale de running
    
    cambiar_estado_a(pcb_mod_tam_arch, BLOCKED, estadoActual(pcb_mod_tam_arch));

    log_info(kernel_logger, "PID: [%d] - Bloqueado por: [%s]",pcb_mod_tam_arch->id, nombre_archivo);
    
    agregar_a_lista_con_sems(pcb_mod_tam_arch, listaBloqueados, m_listaBloqueados);

    thread_args_truncate* argumentos = malloc(sizeof(thread_args_truncate));
    argumentos->pcb = pcb_mod_tam_arch;
    strcpy(argumentos->nombre,nombre_archivo);
    argumentos->tamanio = tamanio_archivo;

    log_error(kernel_logger,"el nombre en argumentos es :%s",argumentos->nombre);
    log_error(kernel_logger,"el tamanio en argumentos es :%d",argumentos->tamanio);

    pthread_create(&hiloTruncate, NULL, (void*) rutina_truncate, (void*) (thread_args*) argumentos);
    pthread_detach(hiloTruncate);

    sem_post(&fin_ejecucion);
    
    
    liberar_ce_string_entero(estructura_mod_tam_archivo);
}

void rutina_truncate(thread_args_truncate* args)
{
    t_pcb* pcb = args->pcb;
    char nombre[100];
    strcpy(nombre,args->nombre);
    uint32_t tamanio = args->tamanio;

    log_error(kernel_logger,"el nombre en rutina es :%s",nombre);
    
    enviar_string_enterov2(file_system_connection, nombre, tamanio, F_TRUNCATE);
    
    uint32_t cod_op = recibir_operacion(file_system_connection);

    log_debug(kernel_logger, "El codigo de operacion que llega dentro de la rutina truncate es: %d", cod_op);

    pthread_mutex_lock(&m_listaBloqueados);
        list_remove_element(listaBloqueados, pcb);
    pthread_mutex_unlock(&m_listaBloqueados);

    cambiar_estado_a(pcb, READY, estadoActual(pcb));
    
    iniciar_nueva_espera_ready(pcb); // hacer cada vez que se mete en la lista de ready
    
    agregar_a_lista_con_sems(pcb, listaReady, m_listaReady);
    
    sem_post(&proceso_en_ready);
}

// ----------------------- Funciones finales ----------------------- //

void destruirSemaforos()
{
    sem_destroy(&proceso_en_ready);
    sem_destroy(&fin_ejecucion);
    sem_destroy(&grado_multiprog);
}

/*
Logs minimos obligatorios TODO
Escribir Archivo: PID: <PID> -  Escribir Archivo: <NOMBRE ARCHIVO> - Puntero <PUNTERO> - Dirección Memoria <DIRECCIÓN MEMORIA> - Tamaño <TAMAÑO> 
*/