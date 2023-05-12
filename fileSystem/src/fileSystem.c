#include "fileSystem.h"

int main(int argc, char ** argv)
{
    // ----------------------- creo el log del fileSystem ----------------------- //
    fileSystem_logger = log_create("./runlogs/fileSystem.log", "KERNEL", 1, LOG_LEVEL_INFO);

    // ----------------------- levanto la configuracion del fileSystem ----------------------- //

    log_info(fileSystem_logger, "levanto la configuracion del fileSystem");
    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }

    fileSystem_config_file = init_config(argv[1]);

    if (fileSystem_config_file == NULL) {
        perror("Ocurrió un error al intentar abrir el archivo config");
        exit(1);
    }

    log_info(fileSystem_logger, "cargo la configuracion del fileSystem");
    
    load_config();
    log_info(fileSystem_logger, "aca no llego si esta la funcion load config puesta");
    log_info(fileSystem_logger, "%s", fileSystem_config.ip_kernel);
    log_info(fileSystem_logger, "%s", fileSystem_config.puerto_kernel);


    end_program(0/*cambiar por conexion*/, fileSystem_logger, fileSystem_config)
    return 0;

}

if((kernel_connection = crear_conexion(fileSystem_config.ip_kernel , fileSystem_config.puerto_kernel)) == -1) {
    log_trace(fileSystem_logger, "No se pudo conectar al servidor de KERNEL");
    exit(2);


socket_servidor_fileSystem = iniciar_servidor(fileSystem_config.puerto_escucha, fileSystem_logger);
    log_trace(fileSystem_logger, "fileSystem inicia el servidor");

    while (1) {

        log_trace(fileSystem_logger, "esperando cliente kernel ");
	    socket_cliente = esperar_cliente(socket_servidor_fileSystem, fileSystem_logger);
            log_trace(fileSystem_logger, "me entro un kernel con este socket: %d", socket_cliente); 

    
        pthread_t atiende_kernel;
            pthread_create(&atiende_kernel, NULL, (void*) recibir_kernel, (void*)socket_cliente);
            pthread_detach(atiende_kernel);

    }


void load_config(void){

    fileSystem_config.ip_memoria = config_get_string_value(fileSystem_config_file, "IP_MEMORIA");
	fileSystem_config.path_superbloque = config_get_string_value(fileSystem_config_file, "PATH_SUPERBLOQUE");
	fileSystem_config.path_bitmap = config_get_string_value(fileSystem_config_file, "PATH_BITMAP");
    fileSystem_config.path_bloques = config_get_string_value(fileSystem_config_file, "PATH_BLOQUES");
    fileSystem_config.path_FCB = config_get_string_value(fileSystem_config_file, "PATH_FCB");
	
	fileSystem_config.puerto_memoria = config_get_int_value(fileSystem_config_file, "PUERTO_MEMORIA");
    fileSystem_config.puerto_escucha = config_get_int_value(fileSystem_config_file, "PUERTO_ESCUCHA");
	fileSystem_config.retardo_acceso_bloque = config_get_int_value(fileSystem_config_file, "RETARDO_ACCESSO_BLOQUE");

    log_info(fileSystem_logger, "config cargada en 'fileSystem_cofig_file'");
}
void end_program(int socket, t_log* log, t_config* config){
    log_destroy(log);
    config_destroy(fileSystem_config_file);
    liberar_conexion(socket);
}