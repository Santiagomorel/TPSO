#include <stdlib.h>
#include <stdio.h>
#include <commons/config.h>
#include <valgrind/valgrind.h>
#include <commons/log.h>
#include "cpu.h"

void funcion(char *str, int i) {
    VALGRIND_PRINTF_BACKTRACE("%s: %d\n", str, i);
}

int main() {
    int conexion_cpu;
	char* ip_memoria;
	char* puerto_memoria;
	char* retardo_instruccion;
	char* puerto_escucha;
	char* tam_max_segmento;

	t_log* logger;
	t_config* config;

    /* ---------------- LOGGING ---------------- */

	logger = iniciar_logger();


	// Usando el logger creado previamente
	// Escribi: "Hola! Soy un log"

	log_info(logger, "Hola! Soy un Log");

	/* ---------------- ARCHIVOS DE CONFIGURACION ---------------- */

	config = iniciar_config();

	// Usando el config creado previamente, leemos los valores del config y los 
	// dejamos en las variables 'ip', 'puerto' y 'valor'

	ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	retardo_instruccion = config_get_string_value(config, "RETARDO_INSTRUCCION");
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	tam_max_segmento = config_get_string_value(config, "TAM_MAX_SEGMENTO");

	

	log_info(logger,"Lei la IP %s , el Puerto Memoria %s ", ip_memoria, puerto_memoria);


	// Loggeamos el valor de config

    /* ---------------- LEER DE CONSOLA ---------------- */

	leer_consola(logger);

	/*----------------------------------------------------------------------------------------------------------------*/


	// Creamos una conexión hacia el servidor
	conexion_cpu = crear_conexion(ip_memoria, puerto_memoria);

	// Enviamos al servidor el valor de ip como mensaje
	enviar_mensaje(ip_memoria, conexion_cpu);

	// Armamos y enviamos el paquete
	paquete(conexion_cpu);

	terminar_programa(conexion_cpu, logger, config);


}



t_log* iniciar_logger(void)
{
	t_log* nuevo_logger;

	if ((nuevo_logger = log_create("tp0.log", "Logs tp0",1 , LOG_LEVEL_INFO)) == NULL){
		printf("No se pudo crear el logger \n");
		exit(1);
	}

	return nuevo_logger;
}

t_config* iniciar_config(void)
{
	t_config* nuevo_config;

	if ((nuevo_config = config_create("/home/utnso/Desktop/TP 2023/tp-2023-1c-EAB-MODEL/cpu/config/cpu.config")) == NULL){
		printf("No se pudo leer la config");
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
	/* Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	  con las funciones de las commons y del TP mencionadas en el enunciado */

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
