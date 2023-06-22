#include "memoria.h"

int main(int argc, char **argv)
{

    log_memoria = log_create("./runlogs/memoria.log", "Memoria", 1, LOG_LEVEL_TRACE);

    /*Estructuras administrativas*/

    // ----------------------- levanto la configuracion de Memoria ----------------------- //

    if (argc < 2)
    {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }

    memoria_config_file = init_config(argv[1]);

    if (memoria_config_file == NULL)
    {
        perror("Ocurrió un error al intentar abrir el archivo config");
        exit(1);
    }

    // ----------------------- cargo la configuracion de memoria ----------------------- //

    log_trace(log_memoria, "cargo la configuracion de Memoria");

    load_config();

    iniciar_semaforos();

    iniciarSegmentacion();

    /*Fin Estructuras Admin*/

    // ----------------------- levanto el servidor de memoria ----------------------- //

    socket_servidor_memoria = iniciar_servidor(memoria_config.puerto_escucha, log_memoria);
    log_trace(log_memoria, "Servidor Memoria listo para recibir al cliente");

    pthread_t atiende_cliente_CPU, atiende_cliente_FILESYSTEM, atiende_cliente_KERNEL;

    log_trace(log_memoria, "esperando cliente CPU");
    socket_cliente_memoria_CPU = esperar_cliente(socket_servidor_memoria, log_memoria);
    pthread_create(&atiende_cliente_CPU, NULL, (void *)recibir_cpu, (void *)socket_cliente_memoria_CPU);
    pthread_detach(atiende_cliente_CPU);

    log_trace(log_memoria, "esperando cliente fileSystem");
    socket_cliente_memoria_FILESYSTEM = esperar_cliente(socket_servidor_memoria, log_memoria);
    pthread_create(&atiende_cliente_FILESYSTEM, NULL, (void *)recibir_fileSystem, (void *)socket_cliente_memoria_FILESYSTEM);
    pthread_detach(atiende_cliente_FILESYSTEM);

    log_trace(log_memoria, "esperando cliente kernel");
    socket_cliente_memoria_KERNEL = esperar_cliente(socket_servidor_memoria, log_memoria);
    pthread_create(&atiende_cliente_KERNEL, NULL, (void *)recibir_kernel, (void *)socket_cliente_memoria_KERNEL);
    pthread_detach(atiende_cliente_KERNEL);

    /*while (1)
    {
        log_trace(log_memoria, "esperando cliente ");
        socket_cliente_memoria = esperar_cliente(socket_servidor_memoria, log_memoria);
        log_trace(log_memoria, "me entro un cliente con este socket: %d", socket_cliente_memoria);

        cod_mod handshake = recibir_handshake(socket_cliente_memoria);

        pthread_t atiende_cliente;

        switch (handshake)
        {
        case KERNEL:
            pthread_create(&atiende_cliente, NULL, (void*) recibir_kernel, (void*)socket_cliente_memoria);
            pthread_detach(atiende_cliente);
            break;
        case CPU:
            pthread_create(&atiende_cliente, NULL, (void*) recibir_cpu, (void*)socket_cliente_memoria);
            pthread_detach(atiende_cliente);
            break;
        case FILESYSTEM:
            pthread_create(&atiende_cliente, NULL, (void*) recibir_fileSystem, (void*)socket_cliente_memoria);
            pthread_detach(atiende_cliente);
            break;
        default:
            log_error(log_memoria, "Modulo desconocido.");
            break;
        }
    }*/
    sem_wait(&finModulo);

    free(MEMORIA_PRINCIPAL);
    end_program(socket_servidor_memoria, log_memoria, memoria_config_file);

    return 0;
}

void load_config(void)
{
    memoria_config.puerto_escucha = config_get_string_value(memoria_config_file, "PUERTO_ESCUCHA");
    memoria_config.tam_memoria = config_get_int_value(memoria_config_file, "TAM_MEMORIA");
    memoria_config.tam_segmento_0 = config_get_int_value(memoria_config_file, "TAM_SEGMENTO_0");
    memoria_config.cant_segmentos = config_get_int_value(memoria_config_file, "CANT_SEGMENTOS");
    memoria_config.retardo_memoria = config_get_int_value(memoria_config_file, "RETARDO_MEMORIA");
    memoria_config.retardo_compactacion = config_get_int_value(memoria_config_file, "RETARDO_COMPACTACION");
    memoria_config.algoritmo_asignacion = config_get_string_value(memoria_config_file, "ALGORITMO_ASIGNACION");
}

void end_program(int socket, t_log *log, t_config *config)
{
    log_destroy(log);
    config_destroy(config);
    liberar_conexion(socket);
}
//KERNEL
void recibir_kernel(int SOCKET_CLIENTE_KERNEL)
{

    enviar_mensaje("recibido kernel", SOCKET_CLIENTE_KERNEL);

    while (1)
    {
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_KERNEL);
        switch (codigoOperacion)
        {
            case INICIAR_ESTRUCTURAS:
                int id_inicio_estructura = recibir_entero(SOCKET_CLIENTE_KERNEL, log_memoria);
                t_proceso* nuevo_proceso = crear_proceso_en_memoria(id_inicio_estructura);

                log_trace(log_memoria, "recibi el op_cod %d INICIAR_ESTRUCTURAS", codigoOperacion);
                log_trace(log_memoria, "creando paquete con tabla de segmentos base");
                
                enviar_tabla_segmentos(SOCKET_CLIENTE_KERNEL, TABLA_SEGMENTOS, nuevo_proceso);

            // enviar_mensaje("enviado nueva tabla de segmentos", SOCKET_CLIENTE_KERNEL);
            enviar_tabla_segmentos(SOCKET_CLIENTE_KERNEL, TABLA_SEGMENTOS, log_memoria);

            break;
        case CREATE_SEGMENT:
            /*1. Que el segmento se cree exitosamente y que la memoria nos devuelva la base del nuevo segmento
              2. Que no se tenga más espacio disponible en la memoria y por lo tanto el proceso tenga que finalizar con error Out of Memory.
              3. Que se tenga el espacio disponible, pero que el mismo no se encuentre contiguo, por lo que se deba compactar, este caso lo vamos a analizar más en detalle,
                ya que involucra controlar las operaciones de File System que se estén ejecutando.*/

            // resp_op codigo_respuesta_seg = crear_segmento() => lo debe agregar a la memoria, o no en caso que no pueda y retornar un codigo de respuesta
            /*
            if(codigo_respuesta_seg){

            }*/

            break;

        case DELETE_SEGMENT:
            // Debe recibir el id del segmento que desea eliminar
            // tabla_segmentos_sin_segmento = borrar_segmento(id_segmento);

            /*Para realizar un DELETE_SEGMENT, el Kernel deberá enviarle a la Memoria el Id del segmento a eliminar y recibirá como respuesta de la Memoria la tabla de segmentos actualizada.
            Nota: No se solicitará nunca la eliminación del segmento 0 o de un segmento inexistente.*/
            break;
        // case COMPACTAR:
        //     sleep(memoria_config.retardo_compactacion);
        //     log_warning(log_memoria, "Solicitud de Compactación");
        //     //compactar();
        //     break;
        // se desconecta kernel
        case -1:
            log_warning(log_memoria, "se desconecto kernel");
            sem_post(&finModulo);
            break;
        default:
            // log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
}
//CPU
void recibir_cpu(int SOCKET_CLIENTE_CPU)
{

    enviar_mensaje("recibido cpu", SOCKET_CLIENTE_CPU);
    log_trace(log_memoria, "recibido cpu");
    while (1)
    {
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_CPU);
        sleep(memoria_config.retardo_memoria);
        switch (codigoOperacion)
        {
        case MENSAJE:
            log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
            recibir_mensaje(SOCKET_CLIENTE_CPU, log_memoria);
            break;

        case MOV_IN: //(Registro, Dirección Fisica): Lee el valor de memoria correspondiente a la Dirección Lógica y lo almacena en el Registro.
            void * direccion_movIn = (void*)recibir_entero(SOCKET_CLIENTE_CPU, log_memoria);
            mov_in(SOCKET_CLIENTE_CPU, direccion_movIn, 0);
            
            // t_paquete* paquete_ok = crear_paquete_op_code(MOV_IN_OK);
            // enviar_paquete(paquete_ok);
            break;
        case MOV_OUT: //(Dirección Fisica, Registro): Lee el valor del Registro y lo escribe en la dirección física de memoria obtenida a partir de la Dirección Lógica.
            int direccion_movOut = recibir_entero(SOCKET_CLIENTE_CPU, log_memoria);
            void * registro = (void*)recibir_string(SOCKET_CLIENTE_CPU, log_memoria);
            ocuparBitMap(direccion_movOut, sizeof(char));
            ocuparMemoria(registro, direccion_movOut, sizeof(char)); 
            enviar_CodOp(SOCKET_CLIENTE_CPU, MOV_OUT_OK);
        case -1:
            log_warning(log_memoria, "se desconecto CPU");
        break;
        default:
            // log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
}
//FILESYSTEM
void recibir_fileSystem(int SOCKET_CLIENTE_FILESYSTEM)
{

    enviar_mensaje("recibido fileSystem", SOCKET_CLIENTE_FILESYSTEM);
    while (1)
    {
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_FILESYSTEM);
        sleep(memoria_config.retardo_memoria);
        switch (codigoOperacion)
        {
        case MENSAJE:
            log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);

            break;

        // case CREATE_FILE:
        //
        //     break;
        case -1:
                log_warning(log_memoria, "se desconecto FILESYSTEM");
            break;
        default:
            log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
}

/* LOGS NECESAIROS Y OBLIGATORIOS
- Creación de Proceso: “Creación de Proceso PID: <PID>”
- Eliminación de Proceso: “Eliminación de Proceso PID: <PID>”
- Creación de Segmento: “PID: <PID> - Crear Segmento: <ID SEGMENTO> - Base: <DIRECCIÓN BASE> - TAMAÑO: <TAMAÑO>”
- Eliminación de Segmento: “PID: <PID> - Eliminar Segmento: <ID SEGMENTO> - Base: <DIRECCIÓN BASE> - TAMAÑO: <TAMAÑO>”
- Inicio Compactación: “Solicitud de Compactación”
- Resultado Compactación: Por cada segmento de cada proceso se deberá imprimir una línea con el siguiente formato:
- “PID: <PID> - Segmento: <ID SEGMENTO> - Base: <BASE> - Tamaño <TAMAÑO>”
- Acceso a espacio de usuario: “PID: <PID> - Acción: <LEER / ESCRIBIR> - Dirección física: <DIRECCIÓN_FÍSICA> - Tamaño: <TAMAÑO> - Origen: <CPU / FS>”
*/

t_list *generar_lista_huecos()
{
    t_list *lista_huecos = list_create();
    t_hueco *nuevoHueco /*= calcularHueco(tabla de procesos o algo)*/;
    list_add(lista_huecos, nuevoHueco);
    return lista_huecos;
}

t_proceso* crear_proceso_en_memoria(int id_proceso){
    t_proceso* nuevoProceso = malloc(sizeof(t_proceso));
    
    nuevoProceso->id = id_proceso;

    generar_tabla_segmentos(nuevoProceso);

    pthread_mutex_lock(&listaProcesos);
    list_add(lista_procesos, nuevoProceso);
    pthread_mutex_unlock(&listaProcesos);

    return nuevoProceso;
}

void generar_tabla_segmentos(t_proceso* proceso){
	t_list* nuevaTabla = list_create();
	t_segmento* nuevoElemento = crear_segmento(0,0,memoria_config.tam_segmento_0);

    list_add(nuevaTabla, nuevoElemento);
    proceso->tabla_segmentos = nuevaTabla;
}


void enviar_tabla_segmentos(int conexion, int codOP, t_proceso* proceso) {
	t_paquete* paquete = crear_paquete_op_code(codOP);

	agregar_entero_a_paquete(paquete, list_size(proceso->tabla_segmentos));

	agregar_tabla_a_paquete(paquete, proceso, log_memoria);

	enviar_paquete(paquete, conexion);

	eliminar_paquete(paquete);
}

//------------------------ SERIALIZACION DE LA TABLA CON MOREL CON MOREL -----------------------------

void agregar_tabla_a_paquete(t_paquete *paquete, t_proceso *proceso, t_log *logger)
{
    t_list *tabla_segmentos = proceso->tabla_segmentos;
    int tamanio = list_size(tabla_segmentos);
    // por la cantidad de elementos que hay dentro de la tabla de segmentos
    // serializar cada uno de sus elementos.
    // serializo del primer elemento de la tabla todos sus componentes y lo coloco en un nuevo t_list*
    // mismo con el segundo y asi hasta la long de la lista, luego serializo la nueva lista.

    for (int i = 0; i < tamanio; i++)
    {
        agregar_entero_a_paquete(paquete, (((t_segmento *)list_get(tabla_segmentos, i))->id_segmento));
        agregar_entero_a_paquete(paquete, (((t_segmento *)list_get(tabla_segmentos, i))->direccion_base));
        agregar_entero_a_paquete(paquete, (((t_segmento *)list_get(tabla_segmentos, i))->tamanio_segmento));
    }

    // te mando todos los segmentos de una, vs del otro lado los tomas y los vas metiendo en un t_list
}


// semaforos

void iniciar_semaforos()
{
    sem_init(&finModulo, 0, 0);
    pthread_mutex_init(&mutexBitMapSegment, NULL);
    pthread_mutex_init(&mutexMemoria, NULL);
    pthread_mutex_init(&mutexIdGlobal, NULL);
    pthread_mutex_init(&listaProcesos, NULL);
}


//
//  CPU
//

void mov_in(int socket_cliente,void* direc_fisica, int size/*sizeof(t_registro)*/){
    //para guido: antes de enviar la direccion fisica, castearla a void* si es q no esta hecha
    char* registro = direc_fisica;

    t_paquete* paquete_ok = crear_paquete_op_code(MOV_IN_OK);
    agregar_string_a_paquete(paquete_ok, registro);
    enviar_paquete(paquete_ok ,socket_cliente);

    eliminar_paquete(paquete_ok);
    //ocuparMemoria(registro, direc_logica, size);
    //ocuparBitMap(direc_logica, size);
}



//
//  SEGMENTACION
//

int iniciarSegmentacion(void)
{
    MEMORIA_PRINCIPAL = malloc(memoria_config.tam_memoria); // el acrhivo de config de ejemplo es 4096 = 2¹²

    if (MEMORIA_PRINCIPAL == NULL)
    {
        // NO SE RESERVO LA MEMORIA
        return 0;
    }

    // LISTAS
    // tablaDeSegmentosDePatotas = list_create();
    // tablaDeSegmentosDeTripulantes = list_create();

    // BITARRAY
    datos = asignarMemoriaBytes(memoria_config.tam_memoria); // LLENA LOS CAMPOS EN 0

    if (datos == NULL)
    {
        // NO SE RESERVO LA MEMORIA
        return 0;
    }

    int tamanio = bitsToBytes(memoria_config.tam_memoria);

    bitMapSegment = bitarray_create_with_mode(datos, tamanio, MSB_FIRST);

    lista_procesos = list_create();

    return 1; // SI FALLA DEVUELVE 0
}

int puedoGuardar(int quieroGuardar)
{ // RECIBE CANT BYTES QUE QUIERO GUARDAR

    int tamanioLibre = tamanioTotalDisponible();
    log_info(log_memoria, "Hay %d espacio libre, quiero guardar %d", tamanioLibre, quieroGuardar);
    if (quieroGuardar <= tamanioLibre)
    {
        return 1;
    }
    else
        return 0; // DEVUELVE 1 SI HAY ESPACIO SUFICIENTE PARA GUARDAR LO QUE QUIERO GUARDAR
}

int tamanioTotalDisponible(void)
{

    int contador = 0;
    int desplazamiento = 0;

    while (desplazamiento < memoria_config.tam_memoria)
    {

        pthread_mutex_lock(&mutexBitMapSegment);
        if ((bitarray_test_bit(bitMapSegment, desplazamiento) == 0))
        {
            contador++;
        }
        pthread_mutex_unlock(&mutexBitMapSegment);
        desplazamiento++;
    }

    return contador;
}

// GuardarEnMemoria
t_segmento *guardarElemento(void *elemento, int size)
{

    t_segmento *unSegmento = malloc(sizeof(t_segmento));
    t_segmento *aux;

    aux = buscarSegmentoSegunTamanio(size); // BUSCA UN SEGMENTO LIBRE PARA GUARDAR LAS TAREAS
    guardarEnMemoria(elemento, aux, size);

    unSegmento->id_segmento = aux->id_segmento;
    unSegmento->direccion_base = aux->direccion_base;
    unSegmento->tamanio_segmento = size;

    free(aux);

    return unSegmento; // DEVUELVE EL SEGMENTO QUE FUE GUARDADO
}

t_segmento* buscarSegmentoSegunTamanio(int size){ 

    t_segmento* segmento;
    t_list* todosLosSegLibres;

    todosLosSegLibres =  buscarSegmentosDisponibles(); //PONE TODOS LOS SEGMENTOS VACIOS EN UNA LISTA 

    t_list* segmentosCandidatos;
    segmentosCandidatos = puedenGuardar( todosLosSegLibres , size); //ME DEVUELVE LOS SEGMENTOS QUE EL TIENEN ESPACIO NECESARIO PARA GUARDAR
    //log_info(logger,"Hay %d segmentos candidatos", list_size(segmentosCandidatos));
    if(list_is_empty(segmentosCandidatos)){
        log_info(log_memoria,"No hay espacio suficiente para guardar, se debe compactar");
        //compactacion(); Descomentar cuando se tenga compactacion
        //segmento = buscarSegmentoSegunTamanio(size);
    }else if(list_size(segmentosCandidatos)== 1){
        segmento = list_get(segmentosCandidatos, 0);
    }else{
        segmento = elegirSegCriterio(segmentosCandidatos, size); //SI EN LA LISTA HAY MAS DE UN SEGMENTO VA A ELEGIR EN QUE SEGMENTO LO VA A GUARDAR SEGUN EL CRITERIO
        

    }
    
    int mismoId(t_segmento* unSegmento){
        return (unSegmento->id_segmento == segmento->id_segmento);
    }
    
    list_remove_by_condition(todosLosSegLibres, (void*)mismoId); //Saco el segmento de la lista asi las puedo eliminar
    
	//list_destroy(todosLosSegLibres);
	//list_destroy(segmentosCandidatos);
	eliminarLista(todosLosSegLibres);
	list_destroy(segmentosCandidatos);
    
    return segmento;
}

t_list* buscarSegmentosDisponibles(){
    
    t_list* segmentos = list_create();
    int base = 0;

    //YA SABEMOS QUE HAY LUGAR, SINO NO HUBIESE     
    
    while(base < (memoria_config.tam_memoria)){
        t_segmento* unSegmento = buscarUnLugarLibre(&base);
        list_add(segmentos,unSegmento);
    }

    return segmentos;    
}

t_segmento* buscarUnLugarLibre(int* base){
    t_segmento* unSegmento = malloc(sizeof(t_segmento));
    int tamanio = 0;
    
    pthread_mutex_lock(&mutexBitMapSegment);
    if(bitarray_test_bit(bitMapSegment, *base) == 1){ //SI EL PRIMERO ES UN UNO, VA A CONTAR CUANDOS ESTAN OCUPADOS DESDE ESE Y CAMBIA LA BASE	
        int desplazamiento = contarEspaciosOcupadosDesde(bitMapSegment, *base); //CUENTA ESPACIOS OCUPADOS DESDE LA ABASE INDICADA
        *base += desplazamiento; //
    }
    pthread_mutex_unlock(&mutexBitMapSegment);
    // ACA YA LA BASE ESTA EN EL PRIMER 0 LIBRE
    tamanio = contarEspaciosLibresDesde(bitMapSegment, *base);  //TAMANIO ES EL TAMANIO DEL SEGMENTO CON 0 LIBRES //ACA MUERE
    
    unSegmento->id_segmento = generarId();
    unSegmento->direccion_base = *base;
    unSegmento->tamanio_segmento = tamanio;
    
    //MUEVO LA BASE PARA BUSCAR EL SIGUIENTE SEGMENTO DESPUES
    *base +=  tamanio;

    return unSegmento;
}

int generarId(){
	
	pthread_mutex_lock(&mutexIdGlobal);
	int t = idGlobal;
	idGlobal++;
	pthread_mutex_unlock(&mutexIdGlobal);
	return t;
}

t_list* puedenGuardar(t_list* segmentos, int size){
   
    t_list* aux;

    int puedoGuardarSeg(t_segmento* segmento){
        return(segmento->tamanio_segmento >= size);
    }
    aux= list_filter(segmentos, (void*)puedoGuardarSeg);

    return aux;
}

void guardarEnMemoria(void *elemento, t_segmento *segmento, int size)
{

    ocuparBitMap(segmento->direccion_base, size);
    ocuparMemoria(elemento, segmento->direccion_base, size);
}

void ocuparMemoria(void *elemento, int base, int size)
{
    pthread_mutex_lock(&mutexMemoria);
    memcpy(MEMORIA_PRINCIPAL + base, elemento, size);
    pthread_mutex_unlock(&mutexMemoria);
}


//Criterio de asignacio
t_segmento* elegirSegCriterio(t_list* segmentos, int size){

    t_segmento* segmento;
    
    log_info(log_memoria,"Elijo segun %s", memoria_config.algoritmo_asignacion);
    
    if(strcmp(memoria_config.algoritmo_asignacion,"FIRST") == 0){//NO ENTRA LA CONCHA DE SU MADRE
        segmento = list_get(segmentos,0); //FIRST FIT DEVUELVE EL PRIMER SEGMENTO DONDE PUEDE GUARDARSE
    }else if(strcmp(memoria_config.algoritmo_asignacion,"BEST") == 0){
    	log_info(log_memoria,"Entre por BEST");
        segmento = segmentoBestFit(segmentos, size); // => Falta agregar la funcion
    }

    return segmento;
}

t_segmento* segmentoBestFit(t_list* segmentos, int size){

    t_segmento* segmento;

    int igualTamanio(t_segmento* segmento){
        return(segmento -> tamanio_segmento == size); //SEGMENTO QUE TENGA EL MISMO TAMANIO QUE LO QUE QUIERO GUARDAR
    }
    t_segmento* segmentoDeIgualTamanio = list_find(segmentos, (void*)igualTamanio); //ME DEVUELVE EL SEGMENTO DE MISMO TAMANIO

    if(segmentoDeIgualTamanio != NULL){
        segmento = segmentoDeIgualTamanio; 
    }else{      //SI NO ENCONTRE UN SEGMENTO DEL MISMO TAMANIO
        segmento = list_get_minimum(segmentos, (void*)segmentoMenorTamanio); //ME DEVUELVE EL SEGMENTO DE MENOR TAMANIO DONDE ENTRE LO QUE QUIERO GUARDAR
    }
    return segmento;
}

t_segmento* segmentoMenorTamanio(t_segmento* segmento, t_segmento* otroSegmento){ //TODO aca estaban sin asterisco

    if(segmento->tamanio_segmento < otroSegmento->tamanio_segmento){
        return segmento;
    }else
        return otroSegmento; 
}

// BitArrays

//asiganrBits/Bytes
char *asignarMemoriaBits(int bits) // recibe bits asigna bytes
{
    char *aux;
    int bytes;
    bytes = bitsToBytes(bits);
    // printf("BYTES: %d\n", bytes);
    aux = malloc(bytes);
    memset(aux, 0, bytes);
    return aux;
}

char *asignarMemoriaBytes(int bytes){
    char *aux;
    aux = malloc(bytes);
    memset(aux, 0, bytes); // SETEA LOS BYTES EN 0
    return aux;
}

int bitsToBytes(int bits){
    int bytes;
    if (bits < 8)
        bytes = 1;
    else
    {
        double c = (double)bits;
        bytes = ceil(c / 8.0);
    }

    return bytes;
}

// bitMaps

void ocuparBitMap(int base, int size)
{

    pthread_mutex_lock(&mutexBitMapSegment);
    for (int i = 0; i < size; i++)
    {
        bitarray_set_bit(bitMapSegment, base + i); // REEMPLAZA LOS 0 POR 1, ASI SABEMOS QUE ESTA OCUPADO
    }
    pthread_mutex_unlock(&mutexBitMapSegment);
}

void liberarBitMap(int base, int size)
{

    pthread_mutex_lock(&mutexBitMapSegment);
    for (int i = 0; i < size; i++)
    {
        bitarray_clean_bit(bitMapSegment, base + i); // REEMPLAZA LOS 1 POR 0, ASI SABEMOS QUE ESTA LIBRE
    }
    pthread_mutex_unlock(&mutexBitMapSegment);
}

int contarEspaciosLibresDesde(t_bitarray *bitmap, int i)
{ // CUENTA LOS 0 DEL BITMAP HASTA EL PRIMER 1 QUE ENCUENTRA

    int contador = 0;

    pthread_mutex_lock(&mutexBitMapSegment);

    while ((bitarray_test_bit(bitmap, i) == 0) && (i < (memoria_config.tam_memoria)))
    {

        // MIENTRAS EL BITMAP EN ESA POSICION SEA 0 Y NO NOS PASEMOS DE LOS LIMITES DE LA MEMORIA
        contador++;
        i++;
    }

    pthread_mutex_unlock(&mutexBitMapSegment);
    return contador;
}

int contarEspaciosOcupadosDesde(t_bitarray *unBitmap, int i)
{ // CUENTA LOS 1 DEL BITMAP HASTA EL PRIMER 0 QUE ENCUENTRA
    int contador = 0;

    while ((bitarray_test_bit(unBitmap, i) == 1) && (i < memoria_config.tam_memoria))
    {
        // MIENTRAS EL BITMAP EN ESA POSICION SEA 1 Y NO NOS PASEMOS DE LOS LIMITES DE LA MEMORIA
        contador++;
        i++;
    }

    return contador;
}


//listas => puede ir en el utils
void eliminarLista(t_list* lista){
	
	list_destroy_and_destroy_elements(lista, (void*)eliminarAlgo);
	

}
void eliminarAlgo(void* algo){
	
	free(algo);
	
}


// Comentarios viejos// => para borrar

// ---------LP entrante----------
// case INICIAR_PCB:
// log_trace(log_memoria, "entro una consola y envio paquete a inciar PCB");                         //particularidad de c : "a label can only be part of a statement"
//     t_pcb* pcb_a_iniciar = iniciar_pcb(SOCKET_CLIENTE);
// log_trace(log_memoria, "pcb iniciado PID : %d", pcb_a_iniciar->id);
//         pthread_mutex_lock(&m_listaNuevos);
//     list_add(listaNuevos, pcb_a_iniciar);
//         pthread_mutex_unlock(&m_listaNuevos);
// log_trace(log_memoria, "log enlistado: %d", pcb_a_iniciar->id);

//     planificar_sig_to_ready();// usar esta funcion cada vez q se agregue un proceso a NEW o SUSPENDED-BLOCKED
//     break;

