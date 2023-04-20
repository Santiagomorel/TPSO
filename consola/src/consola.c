#include "consola.h"


int main(int argc, char ** argv)
{
	char * path = argv[2];
	    printf("%s\n",path);
	int conexion;
	char* ip;
	char* puerto;
	t_log* logger;
	t_config* config;
	char* buffer;
 	FILE* file = fopen(path, "r"); //abro archivo pseudocodigo

/*-------------------------------------Leo pseudocodigo--------------------------------------*/

	/*buffer = readFile(path,file,logger);

	  if(buffer == NULL){
        log_error(logger, "No se encontraron instrucciones.");
        return EXIT_FAILURE; 
    }
	log_info(logger, "Lectura del buffer: \n%s ", buffer);*/
/*-------------------------------------Inicio Logger--------------------------------------*/
	logger = iniciar_logger();
	log_info(logger, "Hola! Soy un log");
/*-------------------------------------Inicio Config--------------------------------------*/
	config = iniciar_config();
	ip = config_get_string_value(config,"IP_KERNEL");
	puerto = config_get_string_value(config,"PUERTO_KERNEL");

	log_info(logger,"IP: %s // port:%s\n",ip,puerto);

	log_info(logger,"Ahora estas en la consola (guardando en consola.log) ");
	leer_consola(logger);

	log_info(logger,"Ahora saliste de la consola");

	/*-------------------------------------Inicio Conexion con Kernel--------------------------------------*/
	conexion = crear_conexion(ip, puerto); 
	
	if(conexion == -1) {
        log_info(logger, "No se pudo conectar al servidor");
        exit(2);
    }
	
    log_info(logger, "Pudimos realizar la conexion");

	enviar_mensaje(ip,conexion);
	enviar_mensaje(puerto,conexion);

	log_info(logger,"Mensaje enviado");
	/*-------------------------------------Paquete--------------------------------------*/
	log_info(logger,"Estas por mandar un paquete");
	//paquete(conexion,buffer);

/*-------------------------------------Fin ejecucion--------------------------------------*/
	terminar_programa(conexion, logger, config, file, buffer);

}

char* readFile(char* path, FILE* file,t_log* logger){
    if(file == NULL){
        log_error(logger, "No se encontro el archivo: %s", path);
        exit(1);
    }

    struct stat stat_file;
    stat(path, &stat_file);

    char* buffer = calloc(1, stat_file.st_size + 1);
    fread(buffer, stat_file.st_size, 1, file);

    return buffer;
}

t_log* iniciar_logger(void)
{
	t_log* nuevo_logger;
	 if((nuevo_logger = log_create("./runlogs/consola.log", "Consola", 1, LOG_LEVEL_INFO)) == NULL){
		printf("No se puede iniciar el logger\n");
		exit(1);
	 }
	 return nuevo_logger;

}

t_config* iniciar_config(void)
{
	t_config* nuevo_config;
	if((nuevo_config = config_create("./config/consola.config")) == NULL){
		printf("No pude leer la config\n");
		exit(2);	
}
	return nuevo_config;
}

void leer_consola(t_log* logger)
{
	char* leido;


	leido = readline("> ");
	
	while(strcmp(leido, "")) {
		log_info(logger, leido);
		leido = readline("> ");
		
	}

	free(leido);
}

void paquete(int conexion,char * buffer)
{

	t_paquete* paquete = crear_paquete();
	agregar_a_paquete(paquete,buffer, strlen(buffer) + 1);

	enviar_paquete(paquete, conexion);
	eliminar_paquete(paquete);
	}






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