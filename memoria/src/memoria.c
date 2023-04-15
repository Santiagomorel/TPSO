#include <stdlib.h>
#include <stdio.h>
#include <commons/config.h>
#include <utils/utils.h>
#include <commons/log.h>
#include <valgrind/valgrind.h>
#include "memoria.h"


 typedef enum
 {
 	MENSAJE,
 	PAQUETE
 }op_code_memoria;

void funcion(char *str, int i) {
    VALGRIND_PRINTF_BACKTRACE("%s: %d\n", str, i);
}

int main(void) {

    t_log* log_memoria = log_create("./runlogs/memoria.log", "Memoria", 1, LOG_LEVEL_INFO);

    int server_memoria_fd = iniciar_servidor();
    log_info(log_memoria, "Servidor Memoria listo para recibir al cliente");
    int cliente_memoria_fd = esperar_cliente(server_memoria_fd);

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
    return EXIT_SUCCESS;
}
void iterator(char* value) {
	log_info(logger,"%s", value);
}
void load_config(void){
    
}
/*int main(void) {
  hello_world();
  return 0;
}*/

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