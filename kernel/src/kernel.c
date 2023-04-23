#include "kernel.h"

int main(int argc, char ** argv)
{
    // ----------------------- creo el log del kernel ----------------------- //
    kernel_logger = init_logger("./runlogs/kernel.log", "KERNEL", 1, LOG_LEVEL_TRACE);

    // ----------------------- levanto la configuracion del kernel ----------------------- //

    log_info(kernel_logger, "levanto la configuracion del kernel");
    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }
    
    kernel_config_file = init_config(argv[1]);

    if (kernel_config_file == NULL) {
        perror("Ocurrió un error al intentar abrir el archivo config");
        exit(1);
    }

    log_info(kernel_logger, "cargo la configuracion del kernel");
    
    load_config();
    
    socket_kernel = iniciar_servidor(kernel_config.puerto_escucha, kernel_logger);
    log_info(kernel_logger, "inicia el servidor");

    int socket_cliente = esperar_cliente(socket_kernel,kernel_logger);
    log_info(kernel_logger, "El socket del cliente tiene el numero %d", socket_cliente);

    recieve_handshake(socket_cliente);

    t_list * lista;
    while(1) {
        int cod_op = recibir_operacion(socket_cliente);
        log_info(kernel_logger, "El codigo de operacion es %d",cod_op);

        switch (cod_op)
        {
        case MENSAJE:
            recibir_mensaje(socket_cliente,kernel_logger);
            break;
        case PAQUETE:
            lista = recibir_paquete(socket_cliente);
            log_info(kernel_logger, "Paquete recibido");
            list_iterate(lista, (void*) iterator);
            break;

        case -1:
            log_error(kernel_logger, "El cliente se desconecto");
            return EXIT_FAILURE;
        default:
            log_warning(kernel_logger, "Operacion desconocida");
            return EXIT_FAILURE;
        }
    }
    //recibir_mensaje(socket_cliente,kernel_logger);

    end_program(0/*cambiar por conexion*/, kernel_logger, kernel_config_file);
    return 0;
}

void iterator(char* value) {
	log_info(kernel_logger,"%s", value);
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
    
    log_info(kernel_logger, "config cargada en 'kernel_cofig_file'");
}
void end_program(int socket, t_log* log, t_config* config){
    log_destroy(log);
    config_destroy(config);
    liberar_conexion(socket);
}