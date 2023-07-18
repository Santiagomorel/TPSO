#include "comunicacion.h"

void procesar_conexion(void *void_args)
{
    t_conexion *args = (t_conexion *)void_args;
    t_log *logger = args->log;
    int cliente_socket = args->socket;
    free(args);

    cod_op cop;

    while (cliente_socket != -1)
    {
        if (recv(cliente_socket, &cop, sizeof(cod_op), 0) != sizeof(cod_op))
        {
            log_warning(logger, "Cliente desconectado!");
            break;
        }

        char f_name[30];
        uint32_t archivo_ok;
        uint32_t pid;
        uint32_t f_size;
        uint32_t dir_fisica;
        uint32_t cant;
        uint32_t offset;
        cod_op cod_op_fs;
        int despl;

        switch (cop)
        {
        case HANDSHAKE_KERNEL:
            aceptar_handshake(logger, cliente_socket, cop);
            break;
        // Errores
        case HANDSHAKE_CONSOLA:
        case HANDSHAKE_CPU:
        case HANDSHAKE_FILESYSTEM:
        case HANDSHAKE_MEMORIA:
            rechazar_handshake(logger, cliente_socket);
            break;
        case ABRIR_ARCHIVO:
            recv(cliente_socket, f_name, sizeof(char[30]), NULL);
            archivo_ok = abrir_archivo(f_name);
            send(cliente_socket, &archivo_ok, sizeof(uint32_t), NULL);
            log_info(logger, "Abrir archivo: %s", f_name);
            break;
        case CREAR_ARCHIVO:
            recv(cliente_socket, f_name, sizeof(char[30]), NULL);
            archivo_ok = crear_archivo(f_name);
            send(cliente_socket, &archivo_ok, sizeof(uint32_t), NULL);
            log_info(logger, "Crear archivo: %s", f_name);
            break;
        case TRUNCAR_ARCHIVO:
            recv(cliente_socket, f_name, sizeof(char[30]), NULL);
            recv(cliente_socket, &f_size, sizeof(uint32_t), NULL);
            truncar_archivo(f_name, f_size);
            archivo_ok = 0;
            send(cliente_socket, &archivo_ok, sizeof(uint32_t), NULL);
            break;
        case LEER_ARCHIVO:
            recv(cliente_socket, &pid, sizeof(uint32_t), NULL);
            recv(cliente_socket, f_name, sizeof(char[30]), NULL);
            recv(cliente_socket, &offset, sizeof(uint32_t), NULL);
            recv(cliente_socket, &dir_fisica, sizeof(uint32_t), NULL);
            recv(cliente_socket, &cant, sizeof(uint32_t), NULL);
            log_info(logger, "Leer archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d" ,f_name, offset, dir_fisica, cant);
            char *stream_leido = eferrid(f_name, offset, cant);
            //char* cadena_parseada = imprimir_cadena(stream_leido, cant);
            //printf("Cadena parseada: %s\n", cadena_parseada);
            void* buffer_leido = malloc(sizeof(uint32_t)*3 + cant + sizeof(cod_op));
            despl = 0;
            cod_op_fs = LEER_ARCHIVO;
            memcpy(buffer_leido + despl, &cod_op_fs, sizeof(cod_op));
            despl += sizeof(cod_op);
            memcpy(buffer_leido + despl, &pid, sizeof(uint32_t));
            despl += sizeof(uint32_t);
            memcpy(buffer_leido + despl, &dir_fisica, sizeof(uint32_t));
            despl += sizeof(uint32_t);
            memcpy(buffer_leido + despl, &cant, sizeof(uint32_t));
            despl += sizeof(uint32_t);
            memcpy(buffer_leido + despl, stream_leido, cant);
            despl += cant;
            
            if(send(socket_memoria, buffer_leido, sizeof(uint32_t)*3 + cant + sizeof(cod_op), NULL) == -1 ) {
            printf("Hubo un problema\n");
            }
            recv(socket_memoria, &archivo_ok, sizeof(uint32_t), NULL);
            send(cliente_socket, &archivo_ok, sizeof(uint32_t), NULL);
            free(buffer_leido);
            free(stream_leido);
            break;
        case ESCRIBIR_ARCHIVO:
            recv(cliente_socket, &pid, sizeof(uint32_t), NULL);
            recv(cliente_socket, f_name, sizeof(char[30]), NULL);
            recv(cliente_socket, &offset, sizeof(uint32_t), NULL);
            recv(cliente_socket, &dir_fisica, sizeof(uint32_t), NULL);
            recv(cliente_socket, &cant, sizeof(uint32_t), NULL);
            log_info(logger, "Escribir archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d" ,f_name, offset, dir_fisica, cant);
            // Pedir datos a memoria
            void* buffer_memoria = malloc(sizeof(cod_op) + sizeof(uint32_t)*3);
            cod_op_fs = ESCRIBIR_ARCHIVO;
            despl = 0;
            memcpy(buffer_memoria, &cod_op_fs, sizeof(cod_op));
            despl += sizeof(cod_op);
            memcpy(buffer_memoria + despl, &pid, sizeof(uint32_t));
            despl += sizeof(uint32_t);
            memcpy(buffer_memoria + despl, &dir_fisica, sizeof(uint32_t));
            despl += sizeof(uint32_t);
            memcpy(buffer_memoria + despl, &cant, sizeof(uint32_t));
            despl += sizeof(uint32_t);
            send(socket_memoria, buffer_memoria, sizeof(cod_op) + sizeof(uint32_t)*3, NULL);
            free(buffer_memoria);

            void* buffer_escritura = malloc(cant);
            recv(socket_memoria, buffer_escritura, cant, NULL);
            eferrait(f_name, offset, cant, buffer_escritura);
            free(buffer_escritura);
            archivo_ok = 0;
            send(cliente_socket, &archivo_ok, sizeof(uint32_t), NULL);

            break;
        default:
            log_error(logger, "Algo anduvo mal en el server de Filesystem");
            log_info(logger, "Cop: %d", cop);
            return;
        }
    }

    log_warning(logger, "El cliente se desconectó del server");
    return;
}