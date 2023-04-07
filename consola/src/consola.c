#include "consola.h"


int main(void)
{
	/*---------------------------------------------------PARTE 2-------------------------------------------------------------*/

	int conexion;
	char* ip;
	char* puerto;
	char* valor;

	t_log* logger;
	t_config* config;

	/* ---------------- LOGGING ---------------- */

	logger = iniciar_logger();
	log_info(logger, "Hola! Soy un log");
	// Usando el logger creado previamente
	// Escribi: "Hola! Soy un log"


	/* ---------------- ARCHIVOS DE CONFIGURACION ---------------- */

	config = iniciar_config();
	ip = config_get_string_value(config,"IP");
	puerto = config_get_string_value(config,"PUERTO");
	valor = config_get_string_value(config,"CLAVE");

	
	// Usando el config creado previamente, leemos los valores del config y los 
	// dejamos en las variables 'ip', 'puerto' y 'valor'

	// Loggeamos el valor de config
	log_info(logger,"IP: %s // port:%s // key:%s\n",ip,puerto,valor);
	/* ---------------- LEER DE CONSOLA ---------------- */
	log_info(logger,"Ahora estas en la consola (guardando en tp0.log) ");
	leer_consola(logger);

	/*---------------------------------------------------PARTE 3-------------------------------------------------------------*/

	// ADVERTENCIA: Antes de continuar, tenemos que asegurarnos que el servidor esté corriendo para poder conectarnos a él
	// Creamos una conexión hacia el servidor
	conexion = crear_conexion(ip, puerto); 
	// Enviamos al servidor el valor de CLAVE como mensaje
	enviar_mensaje(valor,conexion);
	log_info(logger,"Mensaje enviado");
	// Armamos y enviamos el paquete
	log_info(logger,"Estas por mandar un paquete, todo lo que escribas lo recibira el server ");
	paquete(conexion);

	terminar_programa(conexion, logger, config);

	/*---------------------------------------------------PARTE 5-------------------------------------------------------------*/
	// Proximamente
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

	// La primera te la dejo de yapa
	leido = readline("> ");
	// El resto, las vamos leyendo y logueando hasta recibir un string vacío
	while(strcmp(leido, "")) {
		log_info(logger, leido);
		leido = readline("> ");
	}

	// ¡No te olvides de liberar las lineas antes de regresar!
	free(leido);
}

void paquete(int conexion)
{
	// Ahora toca lo divertido!
	t_paquete* paquete = crear_paquete();
	char* leido = readline("> ");
	
	// Leemos y esta vez agregamos las lineas al paquete
	while(strcmp(leido, "")) { 
	
		agregar_a_paquete(paquete, leido, strlen(leido) + 1);
		free(leido);
		leido = readline("> ");
	}
	enviar_paquete(paquete, conexion);
	// ¡No te olvides de liberar las líneas y el paquete antes de regresar!
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
	/* Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	  con las funciones de las commons y del TP mencionadas en el enunciado */
}