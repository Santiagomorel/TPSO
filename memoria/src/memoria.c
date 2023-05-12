#include "memoria.h"

int main(int argc, char ** argv){

    log_memoria = log_create("./runlogs/memoria.log", "Memoria", 1, LOG_LEVEL_TRACE);


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

    // ----------------------- cargo la configuracion de memoria ----------------------- //

    log_trace(log_memoria, "cargo la configuracion de Memoria");
    
    load_config();
    // ----------------------- levanto el servidor de memoria ----------------------- //

    socket_servidor_memoria = iniciar_servidor(memoria_config.puerto_escucha, log_memoria);
    log_trace(log_memoria, "Servidor Memoria listo para recibir al cliente");

    while (1)
    {
        log_trace(log_memoria, "esperando cliente ");
        socket_cliente_memoria = esperar_cliente(socket_servidor_memoria, log_memoria);
        log_trace(log_memoria, "me entro un cliente con este socket: %d", socket_cliente_memoria);

        

        pthread_t atiende_cliente;

        switch (cod_mod)
        {
        case KERNEL:
            pthread_create(&atiende_cliente, NULL, (void*) recibir_kernel, (void*)socket_cliente_memoria);
            pthread_detach(atiende_cliente);
            break;
        case CPU:
            pthread_create(&atiende_cliente, NULL, (void*) recibir_cpu, (void*)socket_cliente_memoria);
            pthread_detach(atiende_cliente);
            break;
        case FILESYSTEM:
            pthread_create(&atiende_cliente, NULL, (void*) recibir_fileSystem, (void*)socket_cliente_memoria);
            pthread_detach(atiende_cliente);
            break;
        default:
            log_error(log_memoria, "Modulo desconocido.");
            break;
        }
    }

    end_program(socket_servidor_memoria, log_memoria, memoria_config_file);
    
    return 0;
}

void iterator(char* value) {
	log_trace(log_memoria,"%s", value);
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

void recibir_kernel(int SOCKET_CLIENTE_KERNEL) {
    int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_KERNEL);
    switch(codigoOperacion)
        {
            case MENSAJE:
                log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
            
                break;
            // ---------LP entrante----------
            // case INICIAR_PCB: 
            // log_trace(log_memoria, "entro una consola y envio paquete a inciar PCB");                         //particularidad de c : "a label can only be part of a statement"
            //     t_pcb* pcb_a_iniciar = iniciar_pcb(SOCKET_CLIENTE);
            // log_trace(log_memoria, "pcb iniciado PID : %d", pcb_a_iniciar->id);
            //         pthread_mutex_lock(&m_listaNuevos);
            //     list_add(listaNuevos, pcb_a_iniciar);
            //         pthread_mutex_unlock(&m_listaNuevos);
            // log_trace(log_memoria, "log enlistado: %d", pcb_a_iniciar->id);

            //     planificar_sig_to_ready();// usar esta funcion cada vez q se agregue un proceso a NEW o SUSPENDED-BLOCKED 
            //     break;

            default:
                log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
                break;
        }
}
void recibir_cpu(int SOCKET_CLIENTE_CPU) {
    int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_CPU);
    switch(codigoOperacion)
        {
            case MENSAJE:
                log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
            
                break;
            // ---------LP entrante----------
            // case INICIAR_PCB: 
            // log_trace(log_memoria, "entro una consola y envio paquete a inciar PCB");                         //particularidad de c : "a label can only be part of a statement"
            //     t_pcb* pcb_a_iniciar = iniciar_pcb(SOCKET_CLIENTE);
            // log_trace(log_memoria, "pcb iniciado PID : %d", pcb_a_iniciar->id);
            //         pthread_mutex_lock(&m_listaNuevos);
            //     list_add(listaNuevos, pcb_a_iniciar);
            //         pthread_mutex_unlock(&m_listaNuevos);
            // log_trace(log_memoria, "log enlistado: %d", pcb_a_iniciar->id);

            //     planificar_sig_to_ready();// usar esta funcion cada vez q se agregue un proceso a NEW o SUSPENDED-BLOCKED 
            //     break;

            default:
                log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
                break;
        }
}
void recibir_fileSystem(int SOCKET_CLIENTE_FILESYSTEM) {
    int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_FILESYSTEM);
    switch(codigoOperacion)
        {
            case MENSAJE:
                log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
            
                break;
            // ---------LP entrante----------
            // case INICIAR_PCB: 
            // log_trace(log_memoria, "entro una consola y envio paquete a inciar PCB");                         //particularidad de c : "a label can only be part of a statement"
            //     t_pcb* pcb_a_iniciar = iniciar_pcb(SOCKET_CLIENTE);
            // log_trace(log_memoria, "pcb iniciado PID : %d", pcb_a_iniciar->id);
            //         pthread_mutex_lock(&m_listaNuevos);
            //     list_add(listaNuevos, pcb_a_iniciar);
            //         pthread_mutex_unlock(&m_listaNuevos);
            // log_trace(log_memoria, "log enlistado: %d", pcb_a_iniciar->id);

            //     planificar_sig_to_ready();// usar esta funcion cada vez q se agregue un proceso a NEW o SUSPENDED-BLOCKED 
            //     break;

            default:
                log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
                break;
        }
}


