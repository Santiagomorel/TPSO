#include "memoria.h"


 typedef enum
 {
 	MENSAJE,
 	PAQUETE
 }op_code_memoria;

void funcion(char *str, int i) {
    VALGRIND_PRINTF_BACKTRACE("%s: %d\n", str, i);
}

int main(int argc, char ** argv){

    log_memoria = log_create("./runlogs/memoria.log", "Memoria", 1, LOG_LEVEL_INFO);


    // ----------------------- levanto la configuracion de Memoria ----------------------- //

    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }

    memoria_config_file = init_config(argv[1]);

    if (memoria_config_file == NULL) {
        perror("Ocurrió un error al intentar abrir el archivo config");
        exit(1);
    }

    log_info(log_memoria, "cargo la configuracion de Memoria");
    
    load_config();
                            ///////SERVIDOR MEMORIA//////

    socket_memoria = iniciar_servidor();
    log_info(log_memoria, "Servidor Memoria listo para recibir al cliente");
    int cliente_memoria_fd = esperar_cliente(socket_memoria);

    t_list* lista_Memoria;
    while (1)
    {
        int cod_op_mem = recibir_operacion(cliente_memoria_fd);
        switch (cod_op_mem){
        case MENSAJE:
            recibir_mensaje(cliente_memoria_fd);
            break;
        case PAQUETE:
            lista_Memoria = recibir_paquete(cliente_memoria_fd);
            log_info(log_memoria, "Me llegaron los siguientes valores:\n");
            list_iterate(lista_Memoria, (void*) iterator);
            break;
        case -1:
            log_warning(logger, "el cliente se desconecto. Terminando servidor");
            return EXIT_FAILURE;
        default:
            log_error(log_memoria, "Operacion desconocida.")
        }
    }
    end_program(0/*cambiar por conexion*/, log_memoria, memoria_config);
    return EXIT_SUCCESS;
}

void iterator(char* value) {
	log_info(logger,"%s", value);
}

void load_config(void){
    memoria_config.puerto_escucha           = config_get_string_value(memoria_config_file, "PUERTO_ESCUCHA");
    memoria_config.tam_memoria              = config_get_string_value(memoria_config_file, "TAM_MEMORIA");
    memoria_config.tam_segmento             = config_get_string_value(memoria_config_file, "TAM_SEGMENTO");
    memoria_config.cant_segmentos           = config_get_string_value(memoria_config_file, "CANT_SEGMENTOS");
    memoria_config.retardo_memoria          = config_get_string_value(memoria_config_file, "RETARDO_MEMORIA");
    memoria_config.retardo_compactacion     = config_get_string_value(memoria_config_file, "RETARDO_COMPACTACION");
    memoria_config.algoritmo_asignacion     = config_get_string_value(memoria_config_file, "ALGORITMO_ASIGNACION");
}

void end_program(int socket, t_log* log, t_config* config){
    log_destroy(log);
    config_destroy(config);
    liberar_conexion(socket);
}
