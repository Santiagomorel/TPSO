#include "consola.h"


int main(int argc, char ** argv)
{
	char * test = argv[2];
	
	int conexion;
	char* ip;
	char* puerto;

	t_log* logger;
	t_config* config;


	logger = iniciar_logger();
	log_info(logger, "Hola! Soy un log");
    printf("%s\n",test);

	config = iniciar_config();
	ip = config_get_string_value(config,"IP_KERNEL");
	puerto = config_get_string_value(config,"PUERTO_KERNEL");

	log_info(logger,"IP: %s // port:%s\n",ip,puerto);

	log_info(logger,"Ahora estas en la consola (guardando en tp0.log) ");
	leer_consola(logger);


	log_info(logger,"Ahora saliste de la consola");
	conexion = crear_conexion(ip, puerto); 

	
	enviar_mensaje(ip,conexion);
	enviar_mensaje(puerto,conexion);
	log_info(logger,"Mensaje enviado");
	
	log_info(logger,"Estas por mandar un paquete, todo lo que escribas lo recibira el server ");
	paquete(conexion);

	terminar_programa(conexion, logger, config);

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

void paquete(int conexion)
{

	t_paquete* paquete = crear_paquete();
	char* leido = readline("> ");
	

	while(strcmp(leido, "")) { 
	
		agregar_a_paquete(paquete, leido, strlen(leido) + 1);
		free(leido);
		leido = readline("> ");
	}
	enviar_paquete(paquete, conexion);
	eliminar_paquete(paquete);
	free(leido);
	}






void terminar_programa(int conexion, t_log* logger, t_config* config)
{
	if (logger !=NULL){
	log_destroy(logger);
	}

	if (config != NULL){
	config_destroy(config);
	}
	liberar_conexion(conexion);
}