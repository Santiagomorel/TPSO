#include "cpu.h"

void funcion(char *str, int i) {
    VALGRIND_PRINTF_BACKTRACE("%s: %d\n", str, i);
}

t_log* logger;

int main() {
    int conexion_cpu;
	char* ip_memoria;
	char* puerto_memoria;
	char* retardo_instruccion;
	char* puerto_escucha;
	char* tam_max_segmento;
	int socket_cpu;

	t_config* config;
	
	
    /* ---------------- LOGGING ---------------- */

	logger = iniciar_logger();

	/* ---------------- ARCHIVOS DE CONFIGURACION ---------------- */
	config = iniciar_config();


	ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	retardo_instruccion = config_get_string_value(config, "RETARDO_INSTRUCCION");
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	tam_max_segmento = config_get_string_value(config, "TAM_MAX_SEGMENTO");



	establecer_conexion(ip_memoria, puerto_memoria, conexion_cpu, config, logger);

	socket_cpu = iniciar_servidor(puerto_escucha, logger);
	esperar_cliente(socket_cpu);


	terminar_programa(conexion_cpu, logger, config);

	return 0;

}

void establecer_conexion(char * ip_memoria, char* puerto_memoria, int conexion_cpu, t_config* config, t_log* logger){

	
	log_info(logger, "Inicio como cliente");

	log_info(logger,"Lei la IP %s , el Puerto Memoria %s ", ip_memoria, puerto_memoria);


	// Loggeamos el valor de config

    /* ---------------- LEER DE CONSOLA ---------------- */

	//leer_consola(logger);

	/*----------------------------------------------------------------------------------------------------------------*/

	// Creamos una conexión hacia el servidor
	conexion_cpu = crear_conexion(ip_memoria, puerto_memoria);

	// Enviamos al servidor el valor de ip como mensaje si es que levanta el cliente
	if((crear_conexion(ip_memoria, puerto_memoria)) == -1){
		log_info(logger, "Error al conectar con Memoria. El servidor no esta activo");
		exit(-1);
	}else{
		enviar_mensaje(ip_memoria, conexion_cpu);
	}

	
	
	
	
	


}



t_log* iniciar_logger(void)
{
	t_log* nuevo_logger;

	if ((nuevo_logger = log_create("./runlogs/cpu.log", "Logs CPU",1 , LOG_LEVEL_TRACE)) == NULL){
		printf("No se pudo crear el logger \n");
		exit(1);
	}

	return nuevo_logger;
}

t_config* iniciar_config(void)
{
	t_config* nuevo_config;

	if ((nuevo_config = config_create("./config/cpu.config")) == NULL){
		printf("No se pudo leer la config");
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



void terminar_programa(int conexion, t_log* logger, t_config* config)
{

	if(logger != NULL){
		log_destroy(logger);
	}

	if(config != NULL){
		config_destroy(config);
	}

	liberar_conexion(conexion);
}





/*int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }

    t_config *config = config_create(argv[1]);
    if (config == NULL) {
        perror("Ocurrió un error al intentar abrir el archivo config");
        exit(1);
    }

    void print_key_and_value(char *key, void *value) {
        printf("%s => %s\n", key, (char *)value);
    }
    dictionary_iterator(config->properties, print_key_and_value);

    config_destroy(config);
    return 0;
}*/
