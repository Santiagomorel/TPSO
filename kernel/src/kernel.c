#include "kernel.h"


int main(int argc, char ** argv)
{
    // ----------------------- creo el log del kernel ----------------------- //
    kernel_logger = log_create("./runlogs/kernel.log", "KERNEL", 1, LOG_LEVEL_TRACE);

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

    end_program(0/*cambiar por conexion*/, kernel_logger, kernel_config_file);
    return 0;
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
    
    // log_info(kernel_logger, "config cargada en 'kernel_cofig_data'");
}
void end_program(int socket, t_log* log, t_config* config){
    log_destroy(log);
    config_destroy(config);
    liberar_conexion(socket);
}