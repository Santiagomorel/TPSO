#include <stdlib.h>
#include <stdio.h>
#include <commons/config.h>
#include <valgrind/valgrind.h>
#include <commons/log.h>
#include "fileSystem.h"

int main(int argc, char ** argv)
{
    // ----------------------- creo el log del kernel ----------------------- //
    t_log * fileSystem_logger = log_create("./runlogs/kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);

    // ----------------------- levanto la configuracion del kernel ----------------------- //

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


	void load_config(void){

    fileSystem_config.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	fileSystem_config.path_superbloque = config_get_string_value(config, "PATH_SUPERBLOQUE");
	fileSystem_config.path_bitmap = config_get_string_value(config, "PATH_BITMAP");
    fileSystem_config.path_bloques = config_get_string_value(config, "PATH_BLOQUES");
    fileSystem_config.path_FCB = config_get_string_value(config, "PATH_FCB");
	
	fileSystem_config.puerto_memoria = config_get_int_value(config, "PUERTO_MEMORIA");
    fileSystem_config.puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");
	fileSystem_config.retardo_acceso_bloque = config_get_int_value(config, "RETARDO_ACCESSO_BLOQUE");

    
    
    // log_info(kernel_logger, "config cargada en 'kernel_cofig_data'");
}
void end_program(int socket, t_log* log, t_config* config){
    log_destroy(log);
    config_destroy(config);
    liberar_conexion(socket);
}