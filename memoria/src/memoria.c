#include "memoria.h"

int main(int argc, char ** argv){

    log_memoria = log_create("./runlogs/memoria.log", "Memoria", 1, LOG_LEVEL_TRACE);

/*Estructuras administrativas*/

    // ----------------------- levanto la configuracion de Memoria ----------------------- //

    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }

    memoria_config_file = init_config(argv[1]);

    if (memoria_config_file == NULL) {
        perror("Ocurrió un error al intentar abrir el archivo config");
        exit(1);
    }


    // ----------------------- cargo la configuracion de memoria ----------------------- //

    log_trace(log_memoria, "cargo la configuracion de Memoria");
    
    load_config();

    iniciarSegmentacion();

/*Fin Estructuras Admin*/
   
    // ----------------------- levanto el servidor de memoria ----------------------- //

    socket_servidor_memoria = iniciar_servidor(memoria_config.puerto_escucha, log_memoria);
    log_trace(log_memoria, "Servidor Memoria listo para recibir al cliente");
    
    pthread_t atiende_cliente_CPU, atiende_cliente_FILESYSTEM, atiende_cliente_KERNEL;

    log_trace(log_memoria, "esperando cliente CPU");
    socket_cliente_memoria_CPU = esperar_cliente(socket_servidor_memoria, log_memoria);
    pthread_create(&atiende_cliente_CPU, NULL, (void*) recibir_cpu, (void*)socket_cliente_memoria_CPU);
    pthread_detach(atiende_cliente_CPU);

    log_trace(log_memoria, "esperando cliente fileSystem");
    socket_cliente_memoria_FILESYSTEM = esperar_cliente(socket_servidor_memoria, log_memoria);
    pthread_create(&atiende_cliente_FILESYSTEM, NULL, (void*) recibir_fileSystem, (void*)socket_cliente_memoria_FILESYSTEM);
    pthread_detach(atiende_cliente_FILESYSTEM);

    log_trace(log_memoria, "esperando cliente kernel");
    socket_cliente_memoria_KERNEL = esperar_cliente(socket_servidor_memoria, log_memoria);
    pthread_create(&atiende_cliente_KERNEL, NULL, (void*) recibir_kernel, (void*)socket_cliente_memoria_KERNEL);
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

void load_config(void){
    memoria_config.puerto_escucha           = config_get_string_value(memoria_config_file, "PUERTO_ESCUCHA");
    memoria_config.tam_memoria              = config_get_int_value(memoria_config_file, "TAM_MEMORIA");
    memoria_config.tam_segmento_0            = config_get_int_value(memoria_config_file, "TAM_SEGMENTO_0");
    memoria_config.cant_segmentos           = config_get_int_value(memoria_config_file, "CANT_SEGMENTOS");
    memoria_config.retardo_memoria          = config_get_int_value(memoria_config_file, "RETARDO_MEMORIA");
    memoria_config.retardo_compactacion     = config_get_int_value(memoria_config_file, "RETARDO_COMPACTACION");
    memoria_config.algoritmo_asignacion     = config_get_string_value(memoria_config_file, "ALGORITMO_ASIGNACION");
}

void end_program(int socket, t_log* log, t_config* config){
    log_destroy(log);
    config_destroy(config);
    liberar_conexion(socket);
}

void recibir_kernel(int SOCKET_CLIENTE_KERNEL) {

    enviar_mensaje("recibido kernel", SOCKET_CLIENTE_KERNEL);
    
    while(1){
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_KERNEL);
        switch(codigoOperacion)
        {
            case INICIAR_ESTRUCTURAS:
                log_trace(log_memoria, "recibi el op_cod %d INICIAR_ESTRUCTURAS", codigoOperacion);
                log_trace(log_memoria, "creando paquete con tabla de segmentos base");
                
                //enviar_mensaje("enviado nueva tabla de segmentos", SOCKET_CLIENTE_KERNEL);
                enviar_tabla_segmentos(SOCKET_CLIENTE_KERNEL, TABLA_SEGMENTOS, log_memoria);

                break;
            
            
            case CREATE_SEGMENT:
                /*1. Que el segmento se cree exitosamente y que la memoria nos devuelva la base del nuevo segmento
                  2. Que no se tenga más espacio disponible en la memoria y por lo tanto el proceso tenga que finalizar con error Out of Memory.
                  3. Que se tenga el espacio disponible, pero que el mismo no se encuentre contiguo, por lo que se deba compactar, este caso lo vamos a analizar más en detalle,
                    ya que involucra controlar las operaciones de File System que se estén ejecutando.*/

                //resp_op codigo_respuesta_seg = crear_segmento() => lo debe agregar a la memoria, o no en caso que no pueda y retornar un codigo de respuesta
                /*
                if(codigo_respuesta_seg){
                    
                }*/

                
                break;
            

            case DELETE_SEGMENT:
                //Debe recibir el id del segmento que desea eliminar            
                //tabla_segmentos_sin_segmento = borrar_segmento(id_segmento);
                
                /*Para realizar un DELETE_SEGMENT, el Kernel deberá enviarle a la Memoria el Id del segmento a eliminar y recibirá como respuesta de la Memoria la tabla de segmentos actualizada.
                Nota: No se solicitará nunca la eliminación del segmento 0 o de un segmento inexistente.*/
                break;



            // se desconecta kernel
            case -1:
                log_warning(log_memoria, "se desconecto kernel");
                sem_post(&finModulo);
                break;
            default:
                //log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
                break;
        }

    }
}
void recibir_cpu(int SOCKET_CLIENTE_CPU) {

    enviar_mensaje("recibido cpu", SOCKET_CLIENTE_CPU);
    log_trace(log_memoria,"recibido cpu");
    while(1){
    int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_CPU);
    switch(codigoOperacion)
        {
            case MENSAJE:
                log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
                recibir_mensaje(SOCKET_CLIENTE_CPU, log_memoria);
                break;

            case MOV_IN: //(Registro, Dirección Lógica): Lee el valor de memoria correspondiente a la Dirección Lógica y lo almacena en el Registro.
                


                //t_paquete* paquete_ok = crear_paquete_op_code(MOV_IN_OK);
                //enviar_paquete(paquete_ok);
                break;
            case MOV_OUT: //(Dirección Lógica, Registro): Lee el valor del Registro y lo escribe en la dirección física de memoria obtenida a partir de la Dirección Lógica.


                //t_paquete* paquete_ok = crear_paquete_op_code(MOV_OUT_OK);
                //enviar_paquete(paquete_ok);
            default:
                //log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
                break;
        }
    }
}
void recibir_fileSystem(int SOCKET_CLIENTE_FILESYSTEM) {

    enviar_mensaje("recibido fileSystem", SOCKET_CLIENTE_FILESYSTEM);

    int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_FILESYSTEM);
    switch(codigoOperacion)
        {
            case MENSAJE:
                log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
                
                break;

            //case CREATE_FILE:
            //
            //    break;
            

            default:
                log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
                break;
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



t_list* generar_lista_huecos(){
    t_list* lista_huecos = list_create();
    t_hueco* nuevoHueco/*= calcularHueco(tabla de procesos o algo)*/;
    list_add(lista_huecos, nuevoHueco);
    return lista_huecos;
}

/* serializacion vieja
void enviar_tabla_segmentos(){
    t_list* tabla_segmentos = list_create();
    t_segmento* segmento_base = crear_segmento(1,1,64); 
    list_add(tabla_segmentos, segmento_base);
    list_map(tabla_segmentos, serializar_segmento);  // < = Problema
    
    //t_paquete* segmentos_paquete = crear_paquete_op_code(TABLA_SEGMENTOS);
    //agregar_a_paquete(segmentos_paquete, tabla_segmentos, sizeof(t_list));
    //enviar_paquete(segmentos_paquete, SOCKET_CLIENTE_KERNEL);
}

void* serializar_segmento(void* segmento)
{
    t_segmento* segmento_actual = (t_segmento*)segmento;
    int bytes = segmento_actual->tamanio_segmento + 2 * sizeof(int);
    void* magic = malloc(bytes);
    int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(segmento_actual->id_segmento), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento, &(segmento_actual->direccion_base), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento, &(segmento_actual->tamanio_segmento), sizeof(int));
    desplazamiento += sizeof(int);

    return magic;
}*/

t_segmento* crear_segmento(int id_seg, int base, int tamanio){
    t_segmento* unSegmento = malloc(sizeof(t_segmento));
    unSegmento->id_segmento = id_seg;
    unSegmento->direccion_base = base;
    unSegmento->tamanio_segmento = tamanio; 
    return unSegmento;
}

void generar_tabla_segmentos(t_proceso* proceso){
	t_list* nuevaTabla = list_create();
    //le agregamos segmento base de prueba
	t_segmento* nuevoElemento = crear_segmento(1,1,memoria_config.tam_segmento_0);

	list_add(nuevaTabla, nuevoElemento);
	proceso->tabla_segmentos = nuevaTabla;
    //return nuevaTabla;
}


void enviar_tabla_segmentos(int conexion, int codOP, t_log* logger) {
	t_paquete *paquete = crear_paquete_op_code(codOP);
    t_proceso* nuevoProceso;

    generar_tabla_segmentos(nuevoProceso);

	agregar_entero_a_paquete(paquete, nuevoProceso->id);

	agregar_entero_a_paquete(paquete, list_size(nuevoProceso->tabla_segmentos));

	agregar_tabla_a_paquete(paquete, nuevoProceso,  logger);

    imprimir_tabla_segmentos(nuevoProceso->tabla_segmentos, logger);

	enviar_paquete(paquete, conexion);

	eliminar_paquete(paquete);
}

//------------------------ SERIALIZACION DE LA TABLA CON MOREL CON MOREL -----------------------------

void agregar_tabla_a_paquete(t_paquete* paquete, t_proceso* proceso ,  t_log* logger){
	t_list* tabla_segmentos = proceso->tabla_segmentos;
	int tamanio = list_size(tabla_segmentos);
	// por la cantidad de elementos que hay dentro de la tabla de segmentos
	// serializar cada uno de sus elementos.
	// serializo del primer elemento de la tabla todos sus componentes y lo coloco en un nuevo t_list*
	// mismo con el segundo y asi hasta la long de la lista, luego serializo la nueva lista.

	for(int i=0; i< tamanio; i++){
		agregar_entero_a_paquete(paquete, (((t_segmento*)list_get(tabla_segmentos, i))->id_segmento));
		agregar_entero_a_paquete(paquete, (((t_segmento*)list_get(tabla_segmentos, i))->direccion_base));
		agregar_entero_a_paquete(paquete, (((t_segmento*)list_get(tabla_segmentos, i))->tamanio_segmento));
	}

	// te mando todos los segmentos de una, vs del otro lado los tomas y los vas metiendo en un t_list
}



t_proceso * recibir_tabla_segmentos(int socket, t_log* logger){
	t_proceso *nuevoProceso = malloc(sizeof(t_proceso));
	int size = 0;
	char *buffer;
	int desp = 0;

	buffer = recibir_buffer(&size, socket);
	log_warning(logger, "Antes de leer el entero");
	nuevoProceso->id = leer_entero(buffer, &desp);
	log_warning(logger, "despues de leer un entero");
	nuevoProceso->tabla_segmentos = leer_tabla_segmentos(buffer, &desp);
	log_warning(logger, "despues de leer la tabla de segmentos");
	free(buffer);

	return nuevoProceso;
}

t_list* leer_tabla_segmentos(char* buffer, int* desp){
	t_list* nuevalista = list_create();
	int tamanio = leer_entero(buffer, desp);
	for(int i=0; i<tamanio; i++){
		int id_segmento = leer_entero(buffer, desp);
		int direccion_base = leer_entero(buffer, desp);
		int tamanio_segmento = leer_entero(buffer, desp);
		t_segmento* nuevoElemento = crear_segmento(id_segmento, direccion_base, tamanio_segmento);
		list_add(nuevalista, nuevoElemento);
	}
	return nuevalista;
}

//semaforos

void iniciar_semaforos(){
    sem_init(&finModulo, 0, 0);
    pthread_mutex_init(&mutexBitMapSegment, NULL);
}

//
//  SEGMENTACION
//

int iniciarSegmentacion(void){
    MEMORIA_PRINCIPAL= malloc(memoria_config.tam_memoria);

    if(MEMORIA_PRINCIPAL == NULL){
        //NO SE RESERVO LA MEMORIA
        return 0;
    }

    //LISTAS
    //tablaDeSegmentosDePatotas = list_create();
    //tablaDeSegmentosDeTripulantes = list_create();

    //BITARRAY
    datos = asignarMemoriaBytes(memoria_config.tam_memoria); //LLENA LOS CAMPOS EN 0

    if(datos == NULL){
        //NO SE RESERVO LA MEMORIA
        return 0;
    }

    int tamanio = bitsToBytes(memoria_config.tam_memoria);

    bitMapSegment = bitarray_create_with_mode(datos,tamanio, MSB_FIRST);

    return 1; //SI FALLA DEVUELVE 0
}

int puedoGuardar(int quieroGuardar){ //RECIBE CANT BYTES QUE QUIERO GUARDAR

    int tamanioLibre = tamanioTotalDisponible();
    log_info(log_memoria, "Hay %d espacio libre, quiero guardar %d", tamanioLibre, quieroGuardar);
    if(quieroGuardar <= tamanioLibre){
        return 1;
    }else return 0; //DEVUELVE 1 SI HAY ESPACIO SUFICIENTE PARA GUARDAR LO QUE QUIERO GUARDAR

    
}


int tamanioTotalDisponible(void){
    
    int contador = 0;
    int desplazamiento = 0 ;

    while (desplazamiento < memoria_config.tam_memoria){

    	pthread_mutex_lock(&mutexBitMapSegment);
        if((bitarray_test_bit(bitMapSegment, desplazamiento) == 0)){
            contador ++;
        }
        pthread_mutex_unlock(&mutexBitMapSegment);
        desplazamiento ++; 
    }

    return contador;
}

//BitArrays

char* asignarMemoriaBits(int bits)//recibe bits asigna bytes
{
	char* aux;
	int bytes;
	bytes = bitsToBytes(bits);
	//printf("BYTES: %d\n", bytes);
	aux = malloc(bytes);
	memset(aux,0,bytes);
	return aux; 
}

char* asignarMemoriaBytes(int bytes){
    char* aux;
    aux = malloc(bytes);
    memset(aux,0,bytes); // SETEA LOS BYTES EN 0
    return aux;
}

int bitsToBytes(int bits){
	int bytes;
	if(bits < 8)
		bytes = 1; 
	else
	{
		double c = (double) bits;
		bytes = ceil(c/8.0);
	}
	
	return bytes;
}

//bitMaps

void ocuparBitMap(t_bitarray* segmentoBitMap, int base,int size){
	
	pthread_mutex_lock(&mutexBitMapSegment);
	for(int i = 0; i < size; i++){
		bitarray_set_bit(bitMapSegment, base + i); //REEMPLAZA LOS 0 POR 1, ASI SABEMOS QUE ESTA OCUPADO
	}
	pthread_mutex_unlock(&mutexBitMapSegment);
}

void liberarBitMap(t_bitarray* segmentoBitMap, int base, int size){
	
	pthread_mutex_lock(&mutexBitMapSegment);
	for(int i = 0; i < size; i++){
		bitarray_clean_bit(bitMapSegment, base + i); //REEMPLAZA LOS 1 POR 0, ASI SABEMOS QUE ESTA LIBRE
	}
	pthread_mutex_unlock(&mutexBitMapSegment);
}

int contarEspaciosLibresDesde(t_bitarray* bitmap, int i){ //CUENTA LOS 0 DEL BITMAP HASTA EL PRIMER 1 QUE ENCUENTRA

    int contador = 0;

    pthread_mutex_lock(&mutexBitMapSegment);

    while((bitarray_test_bit(bitmap, i)==0 ) && (i < (config_valores.tamanio_memoria))) {

        //MIENTRAS EL BITMAP EN ESA POSICION SEA 0 Y NO NOS PASEMOS DE LOS LIMITES DE LA MEMORIA
        contador ++;
        i++;
    }

    pthread_mutex_unlock(&mutexBitMapSegment);
    return contador;
}


int contarEspaciosOcupadosDesde(t_bitarray*unBitmap, int i){ //CUENTA LOS 1 DEL BITMAP HASTA EL PRIMER 0 QUE ENCUENTRA
    int contador =0;
    
    

    while((bitarray_test_bit(unBitmap, i) == 1) && (i < config_valores.tamanio_memoria)){
        //MIENTRAS EL BITMAP EN ESA POSICION SEA 1 Y NO NOS PASEMOS DE LOS LIMITES DE LA MEMORIA
        contador++;
        i++;
    }

    
    return contador;
}


//Comentarios viejos// => para borrar

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