#include "fileSystem.h"

int main(int argc, char ** argv)
{
    // ----------------------- creo el log del filesystem ----------------------- //
    filesystem_logger = log_create("./runlogs/filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_TRACE);

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
    log_info(filesystem_logger, "%s", filesystem_config.ip_memoria);
    log_info(filesystem_logger, "%s", filesystem_config.puerto_memoria);


    if((conexion_memoria = crear_conexion(filesystem_config.ip_memoria , filesystem_config.puerto_memoria)) == -1) {
    log_trace(filesystem_logger, "No se pudo conectar al servidor de memoria");
        exit(2);
    }
    recibir_operacion(conexion_memoria);
    recibir_mensaje(conexion_memoria, filesystem_logger);

    socket_servidor_filesystem = iniciar_servidor(filesystem_config.puerto_escucha, filesystem_logger);
    log_trace(filesystem_logger, "filesystem inicia el servidor");

    log_trace(filesystem_logger, "esperando cliente kernel ");
    socket_cliente_filesystem_kernel = esperar_cliente(socket_servidor_filesystem, filesystem_logger);
    log_trace(filesystem_logger, "me entro un kernel con este socket: %d", socket_cliente_filesystem_kernel); 

    // armar_superbloque();
    // armar_bitmap();
    // armar_bloques();

    
    pthread_t atiende_kernel;

        pthread_create(&atiende_kernel, NULL, (void*) recibir_kernel, (void*) socket_cliente_filesystem_kernel);
        pthread_detach(atiende_kernel);

    while(1) {
        //espera activa
    }
    //posibilidad para no tener que hacer una espera activa hasta que finalice todo
    //hace un wait que reciba un signal cuando le llegue al filesystem un para finalizar el modulo
    //wait(terminar_programa);
        end_program(socket_servidor_filesystem, filesystem_logger, filesystem_config_file);
        return 0;
       
}   


int pertenece(const int lista[],char* archivo , int longitud) {
                    for (int i = 0; i < longitud; i++) {
                        if (lista[i] == archivo) {
                            return 1;
                        }
                    }
                    return 0;
                    }


void recibir_kernel(int SOCKET_CLIENTE_KERNEL) {
    enviar_mensaje("recibido kernel", SOCKET_CLIENTE_KERNEL);
    while(1){
    int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_KERNEL);
    char* archivo = recibir_string(SOCKET_CLIENTE_KERNEL, filesystem_logger);
    switch(codigoOperacion)
        {

            case F_OPEN: 
                int longitud = sizeof lista_fcb / sizeof lista_fcb[0];
                int existe = pertenece(lista_fcb,archivo,longitud);
                if (existe = 1 ){
                    enviar_codOp(SOCKET_CLIENTE_KERNEL, OK);
                }else{
                    crear_fcb(archivo);
                    enviar_mensaje("No tiene FCB, por ende se creo un FCB", SOCKET_CLIENTE_KERNEL);
                    //hacer que sean log trace
                }

                break;

            case F_CLOSE:
                log_trace(filesystem_logger, "recibi el op_cod %d F_CLOSE , De", codigoOperacion);
                enviar_mensaje("envio del F_CLOSE", SOCKET_CLIENTE_KERNEL);
                break;

            case F_SEEK:
                log_trace(filesystem_logger, "recibi el op_cod %d F_SEEK , De", codigoOperacion);
                enviar_mensaje("envio del F_SEEK", SOCKET_CLIENTE_KERNEL);
                break;

            case F_READ:
                log_trace(filesystem_logger, "recibi el op_cod %d F_READ , De", codigoOperacion);
                enviar_mensaje("envio del F_READ", SOCKET_CLIENTE_KERNEL);
                break;

            case F_WRITE:
                log_trace(filesystem_logger, "recibi el op_cod %d F_WRITE , De", codigoOperacion);
                enviar_mensaje("envio del F_WRITE", SOCKET_CLIENTE_KERNEL);
                break;

            case F_TRUNCATE:

                log_trace(filesystem_logger, "recibi el op_cod %d F_TRUNCATE , De", codigoOperacion);
                enviar_mensaje("envio del F_TRUNCATE", SOCKET_CLIENTE_KERNEL);
                break;

            default:
                log_trace(filesystem_logger, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
                break;
        }
}
}

void load_config(void){

    filesystem_config.ip_memoria = config_get_string_value(filesystem_config_file, "IP_MEMORIA");
	filesystem_config.path_superbloque = config_get_string_value(filesystem_config_file, "PATH_SUPERBLOQUE");
	filesystem_config.path_bitmap = config_get_string_value(filesystem_config_file, "PATH_BITMAP");
    filesystem_config.path_bloques = config_get_string_value(filesystem_config_file, "PATH_BLOQUES");
    filesystem_config.path_FCB = config_get_string_value(filesystem_config_file, "PATH_FCB");
	
	filesystem_config.puerto_memoria = config_get_string_value(filesystem_config_file, "PUERTO_MEMORIA");
    filesystem_config.puerto_escucha = config_get_string_value(filesystem_config_file, "PUERTO_ESCUCHA");
	filesystem_config.retardo_acceso_bloque = config_get_string_value(filesystem_config_file, "RETARDO_ACCESSO_BLOQUE");

    log_info(filesystem_logger, "config cargada en 'filesystem_config_file'");
}

void end_program(int socket, t_log* log, t_config* config){
    log_destroy(log);
    config_destroy(filesystem_config_file);
    liberar_conexion(socket);
}

void crear_superbloque(){


    superbloque = malloc(sizeof(t_superbloque));

    char* ruta;
    strcpy(ruta, "../fs/superbloque.dat");

    superbloque -> block_size = 64 ;
    superbloque -> block_count = 65536;
    log_trace(filesystem_logger, "levanto superbloque ");
}

void crear_bitmap(){

    bitmap = malloc(superbloque-> block_count / 8 );

    char* ruta;
    strcpy(ruta, "../fs/bitmap.dat");

    log_trace(filesystem_logger, "levanto bitmap ");
}

void crear_archivo_bloque(){
    archivo_bloques = malloc(sizeof(superbloque-> block_count * superbloque-> block_size));

    char* ruta;
    strcpy(ruta, "../fs/bloque.dat");

    // config_save(archivo_bloques);

    archivo_bloques = list_create();
    log_trace(filesystem_logger, "levanto archivo bloques ");
    // list_add(archivo_bloques, bloque);
}
 


void crear_fcb(char* archivo){

    char* ruta;
    strcpy(ruta, "../fs/");
    strcat(ruta, archivo);
    
    FCB = malloc(sizeof(FileSystem_FCB));
     
    FCB -> nombre_archivo = archivo;
    FCB -> tamanio_archivo = 0;
    FCB -> puntero_directo = 0;
    FCB -> puntero_indirecto = 0;

    list_add(lista_fcb, FCB);

    // config_save(FCB);

    return FCB;

}



// hacer la t list, pasa por parametro blocksize
// hacer el t_bitarray pasa por parametro la cantidad de bloques
// crear el bitarray con funcion create...
// cambiar los valores de los int de los nuevos structs


//El único que ya debe venir creado de antemano es el archivo de superbloque, 
//el resto les recomendaría hacer el chequeo de si ya existen o no (y, en caso de no existir, crearlos), 
//más que nada por comodidad si hace falta limpiar el FS actual y cambiar la configuración del superbloque entre pruebas.


