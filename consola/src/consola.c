#include "consola.h"


int main(int argc, char ** argv)
{
	

	// ----------------------- creo el log de la consola ----------------------- //

	consola_logger = init_logger("./runlogs/consola.log", "Consola", 1, LOG_LEVEL_TRACE);

	// ----------------------- levanto la configuracion de la consola ----------------------- //

    log_info(consola_logger, "levanto la configuracion de la consola");
    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }

	consola_config_file = init_config(argv[1]);

	if (consola_config_file == NULL) {
        perror("Ocurrió un error al intentar abrir el archivo config");
        exit(1);
    }

    log_info(consola_logger, "cargo la configuracion de la consola");

	load_config();
	// ----------------------- levanto el archivo de pseudocodigo de la consola ----------------------- //

	char * path = argv[2];
	int conexion;

	char* buffer;
 	FILE* file = fopen(path, "r"); //abro archivo pseudocodigo

/*-------------------------------------Leo pseudocodigo--------------------------------------*/

	buffer = readFile(path,file);

	if(buffer == NULL){
        log_error(consola_logger, "No se encontraron instrucciones.");
        return EXIT_FAILURE; 
    }
	log_info(consola_logger, "Lectura del buffer: \n%s ", buffer);
/*-------------------------------------Inicio Config--------------------------------------*/

	log_info(consola_logger,"IP: %s // port:%s\n", consola_config.ip_kernel,consola_config.puerto_kernel);

	//  log_info(consola_logger,"Ahora estas en la consola (guardando en consola.log) ");
	//  leer_consola();

	//  log_info(consola_logger,"Ahora saliste de la consola");

	/*-------------------------------------Inicio Conexion con Kernel--------------------------------------*/
	if((conexion = crear_conexion(consola_config.ip_kernel, consola_config.puerto_kernel)) == -1) {
       log_info(consola_logger, "No se pudo conectar al servidor");
        exit(2);
    }

	
	//send_handshake(conexion);
	
    log_info(consola_logger, "Pudimos realizar la conexion con kernel");
	
	/*-------------------------------------Paquete--------------------------------------*/
	// log_info(logger,"Estas por mandar un paquete");
	paquete(conexion, buffer);
	
	// recibir mensaje buena llegada de pseudo <- kernel (antes de armar el pcb)
	// recibir cod finalizacion

	//recibir mensaje llegada de datos desde kernel
	int codOperacion = recibir_operacion(conexion);
	log_trace(consola_logger, "el codigo de operacion que llega es: %d", codOperacion);
	if(codOperacion == MENSAJE){
		recibir_mensaje(conexion,consola_logger);
	}
	else{
		log_info(consola_logger, "Hubo un problema al enviar el paquete");
	}
	// recibir hacer finalizacion

	codOperacion = recibir_operacion(conexion);
	if(codOperacion == FIN_CONSOLA){
        log_info(consola_logger, "se recibio la orden de finalizar consola enviada por el kernel!\n me voy a autodestruir ;(" );
    }else{
        log_info(consola_logger, "no se recibio ninguna orden o la orden %d, aun asi me autodestruire ;( " , codOperacion);
    }

/*-------------------------------------Fin ejecucion--------------------------------------*/
	terminar_programa(conexion, consola_logger, consola_config_file, file, buffer);
	return 0;
}

char* readFile(char* path, FILE* file){
    if(file == NULL){
        log_error(consola_logger, "No se encontro el archivo: %s", path);
        exit(1);
    }

    struct stat stat_file;
    stat(path, &stat_file);

    char* buffer = calloc(1, stat_file.st_size + 1);
    fread(buffer, stat_file.st_size, 1, file);

    return buffer;
}

void load_config(void){
	consola_config.ip_kernel                   = config_get_string_value(consola_config_file, "IP_KERNEL");
	consola_config.puerto_kernel                   = config_get_string_value(consola_config_file, "PUERTO_KERNEL");
	log_info(consola_logger, "config cargada en 'consola_cofig_file'");
}

// void leer_consola(void)
// {
// 	char* leido;


// 	leido = readline("> ");
	
// 	while(strcmp(leido, "")) {
// 		log_info(consola_logger, leido);
// 		leido = readline("> ");
		
// 	}

// 	free(leido);
// }

void enviar_pseudocodigo(int conexion ,int cantLineas ,char* buffer) {
	t_paquete* paquete = crear_paquete_op_code(INICIAR_PCB);
	agregar_a_paquete(paquete, buffer, cantLineas);
}


void paquete(int conexion, char * buffer)
{
	t_paquete* paquete = crear_paquete_op_code(INICIAR_PCB);
	agregar_a_paquete(paquete, buffer, strlen(buffer) + 1);

	enviar_paquete(paquete, conexion);

	eliminar_paquete(paquete);
	}

// procesID payload




void terminar_programa(int conexion, t_log* logger, t_config* config,FILE * file, char * buffer)
{
	if (logger !=NULL){
	log_destroy(logger);
	}

	if (config != NULL){
	config_destroy(config);
	}
	liberar_conexion(conexion);
	fclose(file);
	free(buffer);
}