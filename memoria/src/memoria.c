#include "memoria.h"

int main(int argc, char ** argv){

    log_memoria = log_create("./runlogs/memoria.log", "Memoria", 1, LOG_LEVEL_TRACE);


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
    MEMORIA_PRINCIPAL= malloc(memoria_config.tam_memoria);
    
    // ----------------------- levanto el servidor de memoria ----------------------- //
    
    socket_servidor_memoria = iniciar_servidor(memoria_config.puerto_escucha, log_memoria);
    log_trace(log_memoria, "Servidor Memoria listo para recibir al cliente");
    
    pthread_t atiende_cliente_CPU, atiende_cliente_FILESYSTEM, atiende_cliente_KERNEL;

    // log_trace(log_memoria, "esperando cliente CPU");
    // socket_cliente_memoria_CPU = esperar_cliente(socket_servidor_memoria, log_memoria);
    // pthread_create(&atiende_cliente_CPU, NULL, (void*) recibir_cpu, (void*)socket_cliente_memoria_CPU);
    // pthread_detach(atiende_cliente_CPU);

    // log_trace(log_memoria, "esperando cliente fileSystem");
    // socket_cliente_memoria_FILESYSTEM = esperar_cliente(socket_servidor_memoria, log_memoria);
    // pthread_create(&atiende_cliente_FILESYSTEM, NULL, (void*) recibir_fileSystem, (void*)socket_cliente_memoria_FILESYSTEM);
    // pthread_detach(atiende_cliente_FILESYSTEM);

    log_trace(log_memoria, "esperando cliente kernel");
    socket_cliente_memoria_KERNEL = esperar_cliente(socket_servidor_memoria, log_memoria);
    pthread_create(&atiende_cliente_KERNEL, NULL, (void*) recibir_kernel, (void*)socket_cliente_memoria_KERNEL);
    pthread_detach(atiende_cliente_KERNEL);

    while (1)
    {
    }
    
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
    free(MEMORIA_PRINCIPAL);
    end_program(socket_servidor_memoria, log_memoria, memoria_config_file);
    
    return 0;
}

void load_config(void){
    memoria_config.puerto_escucha           = config_get_string_value(memoria_config_file, "PUERTO_ESCUCHA");
    memoria_config.tam_memoria              = config_get_string_value(memoria_config_file, "TAM_MEMORIA");
    memoria_config.tam_segmento             = config_get_int_value(memoria_config_file, "TAM_SEGMENTO_0");
    memoria_config.cant_segmentos           = config_get_string_value(memoria_config_file, "CANT_SEGMENTOS");
    memoria_config.retardo_memoria          = config_get_string_value(memoria_config_file, "RETARDO_MEMORIA");
    memoria_config.retardo_compactacion     = config_get_string_value(memoria_config_file, "RETARDO_COMPACTACION");
    memoria_config.algoritmo_asignacion     = config_get_string_value(memoria_config_file, "ALGORITMO_ASIGNACION");
}

void end_program(int socket, t_log* log, t_config* config){
    log_destroy(log);
    config_destroy(config);
    liberar_conexion(socket);
}

t_segmento* crear_segmento(int id_seg, int base, int tamanio){
    t_segmento* unSegmento;
    unSegmento->id_segmento = id_seg;
    unSegmento->direccion_base = base;
    unSegmento->tamanio_segmento = tamanio; 
    return unSegmento;
}

void recibir_kernel(int SOCKET_CLIENTE_KERNEL) {

    enviar_mensaje("recibido kernel", SOCKET_CLIENTE_KERNEL);
    while(1){
    int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_KERNEL);
    switch(codigoOperacion)
        {
            case INICIAR_ESTRUCTURAS:
                log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
                log_trace(log_memoria, "creando paquete con tabla de segmentos base");
                t_list* tabla_segmentos = list_create();
                t_segmento* segmento_base = crear_segmento(0,0,64); 
                log_trace(log_memoria, "aca3");
                int descartar = list_add(tabla_segmentos, segmento_base); // ERROR aca hay un segfault
                log_trace(log_memoria, "aca4");
                t_paquete* segmentos_paquete = crear_paquete_op_code(TABLA_SEGMENTOS);
                log_trace(log_memoria, "el codigo de operacion del tpaquete es %d", segmentos_paquete->codigo_operacion);
                agregar_a_paquete(segmentos_paquete, tabla_segmentos, sizeof(&tabla_segmentos));


                enviar_paquete(segmentos_paquete, SOCKET_CLIENTE_KERNEL);
                break;
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

            default:
                log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
                break;
        }
    }
}
void recibir_cpu(int SOCKET_CLIENTE_CPU) {

    enviar_mensaje("recibido cpu", SOCKET_CLIENTE_CPU);
    while(1){
    int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_CPU);
    switch(codigoOperacion)
        {
            case MENSAJE:
                log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
                break;
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

            default:
                log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
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

