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


    sem_wait(&finModulo);
    log_warning(log_memoria, "FINALIZA EL MODULO DE MEMORIA");
    end_program();

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

void end_program()
{
    bitarray_destroy(bitMapSegment);
    free(datos);
    eliminarTablaDeProcesos();
    free(MEMORIA_PRINCIPAL);
    log_destroy(log_memoria);
    config_destroy(memoria_config_file);
    liberar_conexion(socket_servidor_memoria);

}
//KERNEL
void recibir_kernel(int SOCKET_CLIENTE_KERNEL)
{

    enviar_mensaje("recibido kernel", SOCKET_CLIENTE_KERNEL);
    //int cliente_socket = SOCKET_CLIENTE_KERNEL;
    int codigoOP = 0;
    while (codigoOP != -1)
    {
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_KERNEL);
        switch (codigoOperacion)
        {
        case INICIAR_ESTRUCTURAS:
            int id_inicio_estructura = recibir_entero(SOCKET_CLIENTE_KERNEL, log_memoria);
            t_proceso* nuevo_proceso = crear_proceso_en_memoria(id_inicio_estructura);
            log_trace(log_memoria, "recibi el op_cod %d INICIAR_ESTRUCTURAS", codigoOperacion);
            log_trace(log_memoria, "creando paquete con tabla de segmentos base");
            //- Creación de Proceso: “Creación de Proceso PID: <PID>”
            enviar_tabla_segmentos(SOCKET_CLIENTE_KERNEL, TABLA_SEGMENTOS, nuevo_proceso);
        break;
        
        case DELETE_PROCESS:
        int PID = recibir_entero(SOCKET_CLIENTE_KERNEL, log_memoria);
        borrar_proceso(PID);
        break;
        case CREATE_SEGMENT:
            t_3_enteros* estructura_3_enteros = recibir_3_enteros(SOCKET_CLIENTE_KERNEL);
            int id_proceso = estructura_3_enteros->entero1;
            int id_segmento_nuevo = estructura_3_enteros->entero2;
            int tamanio = estructura_3_enteros->entero3;

            if(puedoGuardar(tamanio)==1){
                t_list* segmentosDisponibles = buscarSegmentosDisponibles();

                if(list_is_empty(segmentosDisponibles)){
                   enviar_CodOp(SOCKET_CLIENTE_KERNEL, NECESITO_COMPACTAR);
                }
                else{
                //busco un espacio segun el algoritmo de ordenamiento
                t_segmento* segmento_nuevo = buscarSegmentoSegunTamanio(tamanio);
                segmento_nuevo->id_segmento=id_segmento_nuevo;
                t_proceso* proceso_con_nuevo_segmento = buscar_proceso(id_proceso);
                list_add(proceso_con_nuevo_segmento->tabla_segmentos, segmento_nuevo);
                log_warning(log_memoria,"Creación de Segmento: PID: %d - Crear Segmento: %d - Base: %d - TAMAÑO: %d", id_proceso, id_segmento_nuevo, segmento_nuevo->direccion_base, segmento_nuevo->tamanio_segmento);
                //list_add(proceso_con_nuevo_segmento->tabla_segmentos, segmento_nuevo);
                enviar_paquete_entero(SOCKET_CLIENTE_KERNEL, segmento_nuevo->direccion_base, OK);
                
                log_warning(log_memoria,"envie paquete con entero %d",segmento_nuevo->direccion_base);
                }
            }
            else{
            log_warning(log_memoria, "sin memoria");
            enviar_CodOp(SOCKET_CLIENTE_KERNEL, OUT_OF_MEMORY);
            }
            
            break;

        case DELETE_SEGMENT:
            // Debe recibir el id del segmento que desea eliminar
            
            t_2_enteros* data_delete = recibir_2_enteros(SOCKET_CLIENTE_KERNEL);
            int id_proceso_delete = data_delete->entero1;
            int id_segmento_delete = data_delete->entero2;
            t_proceso* proceso_sin_segmento = borrar_segmento(id_proceso_delete,id_segmento_delete);
            enviar_tabla_segmentos(SOCKET_CLIENTE_KERNEL, TABLA_SEGMENTOS, proceso_sin_segmento);
            break;


         case COMPACTAR:
             sleep(memoria_config.retardo_compactacion);
             log_warning(log_memoria, "Solicitud de Compactación");
             compactacion2();
             break;

        case -1:
            codigoOP = codigoOperacion;
            break;
        // se desconecta kernel
        default:
            log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(log_memoria, "se desconecto kernel");
    sem_post(&finModulo);
}
//CPU
void recibir_cpu(int SOCKET_CLIENTE_CPU)
{

    enviar_mensaje("recibido cpu", SOCKET_CLIENTE_CPU);
    log_trace(log_memoria, "recibido cpu");
    int codigoOP = 0;
    while (codigoOP != -1)
    {
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_CPU);
        //sleep(memoria_config.retardo_memoria);
        switch (codigoOperacion)
        {
        case MENSAJE:
            log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
            recibir_mensaje(SOCKET_CLIENTE_CPU, log_memoria);
            break;

        case MOV_IN: //(Registro, Direc_base ,size): Lee el valor de memoria correspondiente a la Dirección Lógica y lo almacena en el Registro.
            //falta el PID
            //PID|direc_base|size
            //int PID = recibir_entero(SOCKET_CLIENTE_CPU, log_memoria)
            t_3_enteros* movin = recibir_3_enteros(SOCKET_CLIENTE_CPU);
            log_info(log_memoria, "PID: %d - Accion: LEER - Direccion física: %d - Tamaño: %d - Origen: CPU",movin->entero1, movin->entero2,movin->entero3);
            mov_in(SOCKET_CLIENTE_CPU, movin->entero2, movin->entero3);

            break;
        case MOV_OUT: //(Dirección Fisica, Registro): Lee el valor del Registro y lo escribe en la dirección física de memoria obtenida a partir de la Dirección Lógica.
            recive_mov_out* data_mov_out = recibir_mov_out(SOCKET_CLIENTE_CPU);
            //void * registro = (void*)recibir_string(SOCKET_CLIENTE_CPU, log_memoria);
            void* registro = (void*) data_mov_out->registro;
            log_trace(log_memoria,"PID: %d - Acción: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: CPU", data_mov_out->PID, data_mov_out->DF, data_mov_out->size); 
            ocuparBitMap(data_mov_out->DF, data_mov_out->size);
            ocuparMemoria(registro, data_mov_out->DF, data_mov_out->size);

            //falta chequear que el tipo que se pide para los size este bien
            enviar_CodOp(SOCKET_CLIENTE_CPU, MOV_OUT_OK);

            break;
        case -1:
        codigoOP = codigoOperacion;
        break;
        default:
            // log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(log_memoria, "se desconecto CPU");
}
//FILESYSTEM
void recibir_fileSystem(int SOCKET_CLIENTE_FILESYSTEM)
{

    enviar_mensaje("recibido fileSystem", SOCKET_CLIENTE_FILESYSTEM);
    int codigoOP = 0;
    while (codigoOP != -1)
    {
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_FILESYSTEM);
        //sleep(memoria_config.retardo_memoria);
        log_warning(log_memoria, "llegue a fs");
        switch (codigoOperacion)
        {
        case MENSAJE:
            log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);

            break;

        case F_READ:
        log_warning(log_memoria, "entre al fread");
        t_string_3enteros* fRead = recibir_string_3enteros(SOCKET_CLIENTE_FILESYSTEM);
        log_info(log_memoria, "el stream recibido es %d", fRead->string);
        log_info(log_memoria, "PID: %d - Accion: ESCRIBIR - Direccion física: %d - Tamaño: %d - Origen: FS",fRead->entero1, fRead->entero2,fRead->entero3);
        
            break;
        
        case F_WRITE:
        log_warning(log_memoria, "Entre al fwrite");
        t_3_enteros* fWrite = recibir_3_enteros(SOCKET_CLIENTE_FILESYSTEM);
        log_trace(log_memoria,"PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: FS", fWrite->entero1, fWrite->entero2, fWrite->entero3); 
        enviar_CodOp(SOCKET_CLIENTE_FILESYSTEM, F_WRITE_OK);
            //FALTAN COSAS
        break;

        case -1:
        codigoOP = codigoOperacion;
        break;

        default:
            log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(log_memoria, "se desconecto FILESYSTEM");
}

/* LOGS NECESAIROS Y OBLIGATORIOS
- Acceso a espacio de usuario: “PID: <PID> - Acción: <LEER / ESCRIBIR> - Dirección física: <DIRECCIÓN_FÍSICA> - Tamaño: <TAMAÑO> - Origen: <CPU / FS>”
*/


t_proceso* crear_proceso_en_memoria(int id_proceso){
    t_proceso* nuevoProceso = malloc(sizeof(t_proceso));
    
    nuevoProceso->id = id_proceso;

    generar_tabla_segmentos(nuevoProceso);

    pthread_mutex_lock(&listaProcesos);
    list_add(tabla_de_procesos, nuevoProceso);
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
//  KERNEL
//
void liberar_bitmap_segmento(t_segmento* segmento){
// liberar bitmap
    liberarBitMap(segmento->direccion_base, segmento->tamanio_segmento);
}
void eliminar_tabla_segmentos(t_list* tabla_segmentos)
{
// liberar cada segmento de la tabla
    list_iterate(tabla_segmentos, (void*)liberar_bitmap_segmento);
    for (int i = 0; i < list_size(tabla_segmentos); i++)
    {
        free(list_get(tabla_segmentos, i));
    }
    
}
void eliminar_proceso(t_proceso* proceso){
    // Eliminar la tabla de segmentos
    list_clean_and_destroy_elements(proceso->tabla_segmentos, (void*)eliminar_tabla_segmentos);
}

void borrar_proceso(int PID){
        // Eliminar la instancia de la tabla de procesos => list_remove_and_destroy_by_condition
    bool mismoIdProc(t_proceso* unProceso){
    return (unProceso->id == PID);
    }
    list_remove_and_destroy_by_condition(tabla_de_procesos, mismoIdProc , eliminar_proceso);
    log_warning(log_memoria,"Eliminación de Proceso PID: %d", PID);
}


//CREATE_SEGMENT

//void* crear_segmento(int PID, int size){
//    if()
//}

//DELETE_SEGMENT

t_proceso* buscar_proceso(int id_proceso){
    
    bool mismoIdProc(t_proceso* unProceso){
    return (unProceso->id == id_proceso);
    }

    pthread_mutex_lock(&listaProcesos);
    t_proceso* proceso = list_find(tabla_de_procesos, mismoIdProc);
    pthread_mutex_unlock(&listaProcesos);
    return proceso;
}

t_proceso* borrar_segmento(int PID,int id_segmento_elim){
    
    bool mismoIdProc(t_proceso* unProceso){
    return (unProceso->id == PID);
    }
    t_proceso* proceso_del_segmento = list_find(tabla_de_procesos, mismoIdProc);
    bool mismoIdSeg(t_segmento* unSegmento){
        return (unSegmento->id_segmento == id_segmento_elim);
    }
    t_segmento* segmento_a_eliminar = list_find(proceso_del_segmento->tabla_segmentos, mismoIdSeg);
    log_warning(log_memoria,"PID: %d - Eliminar Segmento: %d, Base: %d - TAMAÑO: %d", PID, id_segmento_elim, segmento_a_eliminar->direccion_base, segmento_a_eliminar->tamanio_segmento);
    list_remove_by_condition(proceso_del_segmento->tabla_segmentos, mismoIdSeg);
    liberarBitMap(segmento_a_eliminar->direccion_base,segmento_a_eliminar->tamanio_segmento);
    return proceso_del_segmento;
    
}

//
//  CPU
//

void mov_in(int socket_cliente,int direc_fisica, int size){
    //para guido: antes de enviar la direccion fisica, castearla a void* si es q no esta hecha
    char* registro;
    memcpy(registro, MEMORIA_PRINCIPAL+direc_fisica ,size);
    enviar_paquete_string(socket_cliente,registro,MOV_IN_OK,strlen(registro)+1);
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
        log_error(log_memoria, "no se ha podido reserar meemoria");
        return 0;
    }

    // LISTAS
    tabla_de_procesos = list_create();
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

/*REIVISAR EN CASO DE Q NO FUNCIONEN EL CREATE_SEGMENT


bool necesitoCompactar(int sizeRequerido){
    return  
}
t_list* espaciosLibres(){
    for (int i = 0; i < memoria_config.tam_memoria; i++)
    {
        if(bitarray_test_bit(bitMapSegment, 0){

        }    
    }
    
    
}
*/

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
        log_error(log_memoria,"No se ha compactado correctamente");
        
    }else if(list_size(segmentosCandidatos)== 1){
        segmento = list_get(segmentosCandidatos, 0);
    }else{
        segmento = elegirSegCriterio(segmentosCandidatos, size); //SI EN LA LISTA HAY MAS DE UN SEGMENTO VA A ELEGIR EN QUE SEGMENTO LO VA A GUARDAR SEGUN EL CRITERIO
        

    }
    
    int mismoIdSeg(t_segmento* unSegmento){
        return (unSegmento->id_segmento == segmento->id_segmento);
    }
    
    list_remove_by_condition(todosLosSegLibres, (void*)mismoIdSeg); //Saco el segmento de la lista asi las puedo eliminar
    
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
    
    unSegmento->id_segmento = generarId();// es al pedo 
    unSegmento->direccion_base = *base;
    unSegmento->tamanio_segmento = tamanio;
    
    //MUEVO LA BASE PARA BUSCAR EL SIGUIENTE SEGMENTO DESPUES
    *base +=  tamanio;

    return unSegmento;
}

///  es al pedo ya que la cambio al final de la func

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
        segmento = segmentoBestFit(segmentos, size);
    }else if(strcmp(memoria_config.algoritmo_asignacion,"WORST") == 0){
        segmento = segmentoWorstFit(segmentos, size);
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
t_segmento* segmentoMayorTamanio(t_segmento* segmento, t_segmento* otroSegmento){

    if(segmento->tamanio_segmento > otroSegmento->tamanio_segmento){
        return segmento;
    }else
        return otroSegmento; 
}
t_segmento* segmentoWorstFit(t_list* segmentos, int size){
    
    t_segmento* segmento;

    return segmento = list_get_maximum(segmentos, (void*)segmentoMayorTamanio);
}

//Compactacion
void compactacion2(){
    int base_aux=MEMORIA_PRINCIPAL;
    //libero todo el bitmap
    liberarBitMap(MEMORIA_PRINCIPAL, memoria_config.tam_memoria);
    //De todos los procesos

    
    for (int i = 0; i < list_size(tabla_de_procesos)-1; i++)
    {
        t_proceso* unProceso = list_get(tabla_de_procesos, i);
        //las tablas de segmentos
        for (int j = 0; i < list_size(unProceso->tabla_segmentos); j++)
        {
            t_segmento* unSegmento = list_get(unProceso->tabla_segmentos, j);
            unSegmento->direccion_base = base_aux;
            base_aux += unSegmento->tamanio_segmento +1;

        //- Resultado Compactación: Por cada segmento de cada proceso se deberá imprimir una línea con el siguiente formato:
        log_warning(log_memoria,"PID: %d - Segmento: %d - Base: %d - Tamaño %d", unProceso->id, unSegmento->id_segmento, unSegmento->direccion_base, unSegmento->tamanio_segmento);
        }
        
    }
    ocuparBitMap(MEMORIA_PRINCIPAL, base_aux-1);// les resto 1 por el ultimo +1 de la iteracion del for

}

////
//// Hasta BITARRAYS no se usa nada
////

void compactacion(){
    //BUSCO SEGMENTOS OCUPADOS, CON EL BITMAP EN 1
    t_list* segmentosNoCompactados = buscarSegmentosOcupados();

    //SACO LO QUE TENGO GUARDADO EN ESOS SEGMENTOS
    t_list* cosasDeLosSegmentos = copiarContenidoSeg(segmentosNoCompactados); //HACER

    //LIMPIO EL BITMAP -> TODO EN 0
    liberarBitMap(0, memoria_config.tam_memoria);

    int cantidadS = list_size(segmentosNoCompactados);

    //NUEVA LISTA DE SEGMENTOS
    t_list* segmentosCompactados = list_create();

    //GUARDO LA LISTA COSASDELOSSEGMENTOS EN LA MEMORIA, EN ORDEN, TODOS PEGADOS
    for(int i =0 ; i<cantidadS ; i++){
        //AGARRO UNO
        t_segmento* miSegmento = list_get(segmentosNoCompactados, i);
        
        //LO GUARDO 
        t_segmento* nuevoSegmento = guardarElemento(list_get(cosasDeLosSegmentos,i), miSegmento->tamanio_segmento);
        
        //LO AGREGO A LA NUEVA LISTA
        list_add(segmentosCompactados, nuevoSegmento);
    }

    //ACTUALIZO LOS SEGMENTOS VIEJOS NO COMPACTADOS
    actualizarCompactacion(segmentosNoCompactados, segmentosCompactados); //HACER

    //LIBERO LISTAS
    eliminarLista(cosasDeLosSegmentos);   
    list_destroy(segmentosCompactados);
    list_destroy(segmentosNoCompactados);
    
} 
//CAMBIAR
t_list* buscarSegmentosOcupados(){
    t_list* segmentos = list_create();

    agregarProcesos(segmentos);

    return segmentos;
}
void agregarProcesos(t_list* segmentos){
    int tamanioListaProcesos = list_size(tabla_de_procesos);
    for (int i = 0; i < tamanioListaProcesos-1; i++)
    {
        t_proceso* unProceso = list_get(tabla_de_procesos, i);
        int tamanioListaSegmentosDelProceso = list_size(unProceso->tabla_segmentos);
        for (int j = 0; j < tamanioListaSegmentosDelProceso-1; j++)
        {
            t_segmento* unSegmento = list_get(unProceso->tabla_segmentos, j);
            list_add(segmentos, unSegmento);
        }
        
    }

}

t_list* copiarContenidoSeg(t_list* segmentosNoCompactados){

    t_list* segmentos = list_map(segmentosNoCompactados, (void*)copiarSegmentacion);
    return segmentos;

}

//CHEQUEAR

void* copiarSegmentacion(t_segmento* unSegmento){
	
	void* algo;
    //if(unSegmento->limite == 21){       //TRIPU
    //    algo = malloc(sizeof(t_tcb));
    //}else if(unSegmento->limite == 8){  //PATOTA
    //    algo = malloc(sizeof(t_pcb));
    //}else{                              //TAREAS
    //    algo = malloc(unSegmento->limite);
    //}

    algo = malloc(unSegmento->tamanio_segmento);
    //CHEQUEAR
	pthread_mutex_lock(&mutexMemoria);
	memcpy(algo,MEMORIA_PRINCIPAL + unSegmento->direccion_base, unSegmento->tamanio_segmento); //COPIA EN ALGO DESDE HASTA TAL LUGAR
	//CHEQUEAR
    pthread_mutex_unlock(&mutexMemoria);
    
    return algo;
}


//CHEQUEAR
//VER LA ACTUALIZACION DE LA COMPACTACION



void actualizarCompactacion(t_list* segmentosNoCompactados, t_list* segmentosCompactados){
    for(int i = 0; i<list_size(segmentosNoCompactados);i++){
        actualizarCadaSegmento(list_get(segmentosNoCompactados, i), list_get(segmentosCompactados,i));
    }
}
void actualizarCadaSegmento(t_segmento* segmentoViejo, t_segmento* segmentoNuevo){
    pthread_mutex_lock(&mutexMemoria);		
	log_info(log_memoria, "El segmento %d ahora tiene base %p",segmentoViejo->id_segmento, MEMORIA_PRINCIPAL + segmentoNuevo->direccion_base);
	pthread_mutex_unlock(&mutexMemoria);
    
    //CAMBIAR A FUNCIONES QUE SIRVAN O BORRAR
    //actualizarSegmento(segmentoViejo, segmentoNuevo); //si esta lo cambia, sino no hace nada
}

//void actualizarSegmento(t_segmento* viejo, t_segmento* nuevo){
//}


//void actualizarBase(t_segmento* segmento, t_segmento* otroSeg){
//
//    segmento->base= otroSeg->base;
//}




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


void eliminarTablaDeProcesos(){
    list_destroy_and_destroy_elements(tabla_de_procesos, (void*)eliminarTablaDeSegmentos);
}
void eliminarTablaDeSegmentos(t_proceso* proceso){
    list_destroy_and_destroy_elements(proceso->tabla_segmentos, (void*)free);
    free(proceso);
}