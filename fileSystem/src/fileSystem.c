#include "filesystem.h"

int main(int argc, char ** argv)
{
    // ----------------------- creo el log del filesystem ----------------------- //
    filesystem_logger = log_create("./runlogs/filesystem.log", "KERNEL", 1, LOG_LEVEL_INFO);

    // ----------------------- levanto la configuracion del filesystem ----------------------- //

    log_info(filesystem_logger, "levanto la configuracion del filesystem");
    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }

    filesystem_config_file = init_config(argv[1]);

    if (filesystem_config_file == NULL) {
        perror("Ocurrió un error al intentar abrir el archivo config");
        exit(1);
    }

    log_info(filesystem_logger, "cargo la configuracion del filesystem");
    
    load_config();
    armar_estructuras_iniciales();
    log_info(filesystem_logger, "aca no llego si esta la funcion load config puesta");
    log_info(filesystem_logger, "%s", filesystem_config.ip_memoria);
    log_info(filesystem_logger, "%s", filesystem_config.puerto_memoria);


}

    if((memoria_connection = crear_conexion(filesystem_config.ip_memoria , filesystem_config.puerto_memoria)) == -1) {
    log_trace(filesystem_logger, "No se pudo conectar al servidor de memoria");
        exit(2);


    socket_servidor_filesystem = iniciar_servidor(filesystem_config.puerto_escucha, filesystem_logger);
    log_trace(filesystem_logger, "filesystem inicia el servidor");

    while (1) {

        log_trace(filesystem_logger, "esperando cliente kernel ");
	    socket_cliente_filesystem = esperar_cliente(socket_servidor_filesystem, filesystem_logger);
            log_trace(filesystem_logger, "me entro un kernel con este socket: %d", socket_cliente_filesystem); 

    
        pthread_t atiende_kernel;

            pthread_create(&atiende_kernel, NULL, (void*) recibir_kernel, (void*)socket_cliente_filesystem);
            pthread_detach(atiende_kernel);


    }

        end_program(socket_servidor_filesystem, filesystem_logger, filesystem_config){
        return 0;
        }   
}   


void recibir_kernel(int socket_cliente) {
    int codigoDeOperacion = recibir_operacion(SOCKET_CLIENTE_KERNEL);
    switch(codigoDeOperacion)
        {
            case MENSAJE:
                log_trace(log_memoria, "recibi el op_cod %d MENSAJE , De", De);
            
                break;

            default:
                log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", De);
                break;
        }
}


void load_config(void){




    filesystem_config.ip_memoria = config_get_string_value(filesystem_config_file, "IP_MEMORIA");
	filesystem_config.path_superbloque = config_get_string_value(filesystem_config_file, "PATH_SUPERBLOQUE");
	filesystem_config.path_bitmap = config_get_string_value(filesystem_config_file, "PATH_BITMAP");
    filesystem_config.path_bloques = config_get_string_value(filesystem_config_file, "PATH_BLOQUES");
    filesystem_config.path_FCB = config_get_string_value(filesystem_config_file, "PATH_FCB");
	
	filesystem_config.puerto_memoria = config_get_int_value(filesystem_config_file, "PUERTO_MEMORIA");
    filesystem_config.puerto_escucha = config_get_int_value(filesystem_config_file, "PUERTO_ESCUCHA");
	filesystem_config.retardo_acceso_bloque = config_get_int_value(filesystem_config_file, "RETARDO_ACCESSO_BLOQUE");

    log_info(filesystem_logger, "config cargada en 'filesystem_cofig_file'");
}

void end_program(int socket, t_log* log, t_config* config){
    log_destroy(log);
    config_destroy(filesystem_config_file);
    liberar_conexion(socket);
}

void armar_estructuras_iniciales(){
    Filesystem_superbloque* superbloque = malloc(sizeof(Filesystem_superbloque));
     
}

t_list* archivo_superbloque = list_create();

t_bitarray * bitarray_create(char)


// hacer el receive_handshake
// hacer la t list, pasa por parametro blocksize
// hacer el t_bitarray pasa por parametro la cantidad de bloques
// crear el bitarray con funcion create...
// cambiar los valores de los int de los nuevos structs
