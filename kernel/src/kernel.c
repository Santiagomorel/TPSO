#include "kernel.h"

int main(int argc, char ** argv)
{
    // ----------------------- creo el log del kernel ----------------------- //

    kernel_logger = init_logger("./runlogs/kernel.log", "KERNEL", 1, LOG_LEVEL_TRACE);

    // ----------------------- levanto la configuracion del kernel ----------------------- //

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

    // ----------------------- cargo la configuracion del kernel ----------------------- //

    log_trace(kernel_logger, "cargo la configuracion del kernel");
    
    load_config();
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

    while (1) {

        log_trace(kernel_logger, "esperando cliente consola ");
	    socket_cliente = esperar_cliente(socket_servidor_kernel, kernel_logger);
            log_trace(kernel_logger, "me entro una consolas con este socket: %d", socket_cliente); 

    
        pthread_t atiende_consola;
            pthread_create(&atiende_consola, NULL, (void*) recibir_consola, (void*)socket_cliente);
            pthread_detach(atiende_consola);

    }

    // int socket_cliente = esperar_cliente(socket_kernel,kernel_logger);
    // log_trace(kernel_logger, "El socket del cliente tiene el numero %d", socket_cliente);

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

    // end_program(0/*cambiar por conexion*/, kernel_logger, kernel_config_file);
    return 0;
}

void iterator(char* value) {
	log_trace(kernel_logger,"%s", value);
}

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
    
    kernel_config.ip_kernel                    = config_get_string_value(kernel_config_file, "IP_KERNEL");
    kernel_config.puerto_kernel                = config_get_string_value(kernel_config_file, "PUERTO_KERNEL");
    
    log_trace(kernel_logger, "config cargada en 'kernel_cofig_file'");
}
void end_program(int socket, t_log* log, t_config* config){
    log_destroy(log);
    config_destroy(config);
    liberar_conexion(socket);
}

void recibir_consola(int SOCKET_CLIENTE) {
    int codigoOperacion = recibir_operacion(SOCKET_CLIENTE);

        switch(codigoOperacion)
        {
            case MENSAJE:
                log_trace(kernel_logger, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
            
                break;
            // ---------LP entrante----------
            // case INICIAR_PCB: 
            // log_trace(kernel_logger, "entro una consola y envio paquete a inciar PCB");                         //particularidad de c : "a label can only be part of a statement"
            //     t_pcb* pcb_a_iniciar = iniciar_pcb(SOCKET_CLIENTE);
            // log_trace(kernel_logger, "pcb iniciado PID : %d", pcb_a_iniciar->id);
            //         pthread_mutex_lock(&m_listaNuevos);
            //     list_add(listaNuevos, pcb_a_iniciar);
            //         pthread_mutex_unlock(&m_listaNuevos);
            // log_trace(kernel_logger, "log enlistado: %d", pcb_a_iniciar->id);

            //     planificar_sig_to_ready();// usar esta funcion cada vez q se agregue un proceso a NEW o SUSPENDED-BLOCKED 
            //     break;

            default:
                log_trace(kernel_logger, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
                break;
        }
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


Orden de inicio de servidores: Memoria -> FileSystem/CPU (Ambos tienen que estar levantados) -> Kernel -> Consola

Consola en multiples instancias va a: levantar su configuracion inicial -> conectarse con el servidor de kernel -> hacer un handshake -> mandar el paquete con el pseudocodigo ->
recibir un mensaje de llegada de pseudocodigo -> esperar a otro mensaje como SUCCES - SEG_FAULT - OUT_OF_MEMORY -> finaliza el programa de consola

kernel va a: levantar su configuracion inicial -> levantar el servidor -> conectarse con los servidores de CPU - Memoria - FileSystem ->
recibir Multiplexando clientes consola -> hacer el hanshake -> recibir el archivo de pseudocodigo y enviar mensaje de buena llegada -> ejecutar el proceso ->
enviar a consola el mensaje de finalizacion ya sea SUCCES - SEG_FAULT - OUT_OF_MEMORY
*/