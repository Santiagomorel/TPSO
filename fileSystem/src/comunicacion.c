#include "comunicacion.h"


void procesar_conexion()
{
    socket_servidor_filesystem = iniciar_servidor(logger_filesystem, PUERTO_ESCUCHA_FILESYSTEM);
    socket_fs = esperar_cliente(socket_servidor_filesystem, logger_filesystem);
    enviar_mensaje("Kernel conectado",socket_fs);
    

    t_log *logger =logger_filesystem;
    int cliente_socket = socket_fs;
    

    int cop;

    while (cliente_socket != -1)
    {
        if (recv(cliente_socket, &cop, sizeof(int), 0) != sizeof(int))
        {
            log_warning(logger, "Cliente desconectado!");
            break;
        }

        char* f_name;
        uint32_t archivo_ok;
        uint32_t pid;
        uint32_t f_size;
        uint32_t dir_fisica;
        uint32_t cant;
        uint32_t offset;
        int cod_op_fs;
        int despl;

        switch (cop)
        {
        case F_OPEN:
            char* nombre_archivo = recibir_string(cliente_socket, logger);

            archivo_ok = abrir_archivo(nombre_archivo);


            if(archivo_ok){
                enviar_CodOp(cliente_socket, NO_EXISTE_ARCHIVO);
            }else{
                enviar_CodOp(cliente_socket, EXISTE_ARCHIVO);
            }


            log_info(logger, "Abrir archivo: %s", nombre_archivo);
            break;
        case F_CREATE:
            char* nombre_archivo2 = recibir_string(cliente_socket, logger);

            crear_archivo(nombre_archivo);

            log_info(logger, "Crear archivo: %s", nombre_archivo);
            break;
        case F_TRUNCATE:
            t_string_entero* estructura = recibir_string_entero(cliente_socket);
            truncar_archivo(estructura->string, (uint32_t)estructura->entero1);
            break;
        case F_READ:
            t_string_4enteros* estructura_string_4enteros_l = recibir_string_4enteros(cliente_socket);
            
            char* f_name = estructura_string_4enteros_l->string;
            uint32_t pid = estructura_string_4enteros_l->entero4;
            uint32_t offset = estructura_string_4enteros_l->entero1;
            uint32_t dir_fisica = estructura_string_4enteros_l->entero3;
            uint32_t cant = estructura_string_4enteros_l->entero2;


            log_info(logger, "Leer archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d" ,f_name, offset, dir_fisica, cant);
            char *stream_leido = eferrid(f_name, offset, cant);
            //char* cadena_parseada = imprimir_cadena(stream_leido, cant);
            //printf("Cadena parseada: %s\n", cadena_parseada);
            
            /*
            void* buffer_leido = malloc(sizeof(uint32_t)*3 + cant + sizeof(int));
            despl = 0;
            cod_op_fs = F_READ;
            memcpy(buffer_leido + despl, &cod_op_fs, sizeof(int));
            despl += sizeof(int);
            memcpy(buffer_leido + despl, &pid, sizeof(uint32_t));
            despl += sizeof(uint32_t);
            memcpy(buffer_leido + despl, &dir_fisica, sizeof(uint32_t));
            despl += sizeof(uint32_t);
            memcpy(buffer_leido + despl, &cant, sizeof(uint32_t));
            despl += sizeof(uint32_t);
            memcpy(buffer_leido + despl, stream_leido, cant);
            despl += cant;
*/

            enviar_string_3enteros(socket_memoria, stream_leido, pid, dir_fisica, cant, F_READ);

            /*
            if(send(socket_memoria, buffer_leido, sizeof(uint32_t)*3 + cant + sizeof(int), NULL) == -1 ) {
            printf("Hubo un problema\n");
            }*/
            
            //recv(socket_memoria, &archivo_ok, sizeof(uint32_t), NULL);

            int cod_op = recibir_operacion(socket_memoria);
            enviar_CodOp(cliente_socket, cod_op);

            //free(buffer_leido);
            free(stream_leido);
            break;
        case F_WRITE:
            t_string_4enteros* estructura_string_4enteros_e = recibir_string_4enteros(cliente_socket);

            char* f_name2= estructura_string_4enteros_e->string;
            uint32_t pid2 = estructura_string_4enteros_e->entero4;
            uint32_t offset2 = estructura_string_4enteros_e->entero1;
            uint32_t dir_fisica2 = estructura_string_4enteros_e->entero3;
            uint32_t cant2 = estructura_string_4enteros_e->entero2;

            log_info(logger, "Escribir archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d" ,f_name2, offset2, dir_fisica2, cant2);
            // Pedir datos a memoria
            /*
            void* buffer_memoria = malloc(sizeof(int) + sizeof(uint32_t)*3);
            cod_op_fs = F_WRITE;
            despl = 0;
            memcpy(buffer_memoria, &cod_op_fs, sizeof(int));
            despl += sizeof(int);
            memcpy(buffer_memoria + despl, &pid2, sizeof(uint32_t));
            despl += sizeof(uint32_t);
            memcpy(buffer_memoria + despl, &dir_fisica2, sizeof(uint32_t));
            despl += sizeof(uint32_t);
            memcpy(buffer_memoria + despl, &cant2, sizeof(uint32_t));
            despl += sizeof(uint32_t);
*/

            enviar_3enteros(socket_memoria, pid2, dir_fisica2, cant2, F_WRITE);

            //send(socket_memoria, buffer_memoria, sizeof(int) + sizeof(uint32_t)*3, NULL);
            //free(buffer_memoria);

            void* buffer_escritura = malloc(cant2);
            recv(socket_memoria, buffer_escritura, cant2, NULL);
            eferrait(f_name2, offset2, cant2, buffer_escritura);
            free(buffer_escritura);
            
            enviar_CodOp(cliente_socket, OK);

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