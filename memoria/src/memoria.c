#include "memoria.h"

int main(int argc, char **argv)
{

    log_memoria = log_create("./runlogs/memoria.log", "Memoria", 1, LOG_LEVEL_TRACE);

    /*Estructuras administrativas*/

    // ----------------------- levanto la configuracion de Memoria ----------------------- //

    if (argc < 2)
    {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }

    memoria_config_file = init_config(argv[1]);

    if (memoria_config_file == NULL)
    {
        perror("Ocurrió un error al intentar abrir el archivo config");
        exit(1);
    }

    // ----------------------- cargo la configuracion de memoria ----------------------- //

    log_trace(log_memoria, "antes de cargar la configuracion de Memoria");

    load_config();

    log_trace(log_memoria, "cargo la configuracion de Memoria");

    levantar_estructuras_administrativas();

    log_trace(log_memoria, "levanto las estructuras");
    pthread_mutex_init(&mutex_memoria, NULL);
    sem_init(&finModulo, 0, 0);

    log_trace(log_memoria, "puerto escucha %d", memoria_config.puerto_escucha);

    socket_servidor_memoria = iniciar_servidor(memoria_config.puerto_escucha, log_memoria);
    log_trace(log_memoria, "Servidor Memoria listo para recibir al cliente");

    pthread_t atiende_cliente_CPU, atiende_cliente_FILESYSTEM, atiende_cliente_KERNEL;

    log_trace(log_memoria, "esperando cliente CPU");
    socket_cliente_memoria_CPU = esperar_cliente(socket_servidor_memoria, log_memoria);
    pthread_create(&atiende_cliente_CPU, NULL, (void *)recibir_cpu, (void *)socket_cliente_memoria_CPU);
    pthread_detach(atiende_cliente_CPU);

    log_trace(log_memoria, "esperando cliente fileSystem");
    socket_cliente_memoria_FILESYSTEM = esperar_cliente(socket_servidor_memoria, log_memoria);
    pthread_create(&atiende_cliente_FILESYSTEM, NULL, (void *)recibir_fileSystem, (void *)socket_cliente_memoria_FILESYSTEM);
    pthread_detach(atiende_cliente_FILESYSTEM);

    log_trace(log_memoria, "esperando cliente kernel");
    socket_cliente_memoria_KERNEL = esperar_cliente(socket_servidor_memoria, log_memoria);
    pthread_create(&atiende_cliente_KERNEL, NULL, (void *)recibir_kernel, (void *)socket_cliente_memoria_KERNEL);
    pthread_detach(atiende_cliente_KERNEL);




/* INICIOS
    iniciar_semaforos();

    iniciarSegmentacion();

    //Fin Estructuras Admin
    
    // ----------------------- levanto el servidor de memoria ----------------------- //

    socket_servidor_memoria = iniciar_servidor(memoria_config.puerto_escucha, log_memoria);
*/
    //log_trace(log_memoria, "Servidor Memoria listo para recibir al cliente");
    //sem_wait(&finModulo);
    sem_wait(&finModulo);
    log_warning(log_memoria, "FINALIZA EL MODULO DE MEMORIA");
    end_program();

    return 0;
}

void load_config(void)
{
    memoria_config.puerto_escucha = config_get_string_value(memoria_config_file, "PUERTO_ESCUCHA");
    memoria_config.tam_memoria = config_get_int_value(memoria_config_file, "TAM_MEMORIA");
    memoria_config.tam_segmento_0 = config_get_int_value(memoria_config_file, "TAM_SEGMENTO_0");
    memoria_config.cant_segmentos = config_get_int_value(memoria_config_file, "CANT_SEGMENTOS");
    memoria_config.retardo_memoria = config_get_int_value(memoria_config_file, "RETARDO_MEMORIA");
    memoria_config.retardo_compactacion = config_get_int_value(memoria_config_file, "RETARDO_COMPACTACION");
    char * algo_asig = config_get_string_value(memoria_config_file, "ALGORITMO_ASIGNACION");

    if (strcmp(algo_asig, "FIRST") == 0)
    {
        memoria_config.algoritmo_asignacion = FIRST;
    } else if (strcmp(algo_asig, "BEST") == 0)
    {
        memoria_config.algoritmo_asignacion = BEST;
    } else if (strcmp(algo_asig, "WORST") == 0)
    {
        memoria_config.algoritmo_asignacion = WORST;
    } else {
        log_error(log_memoria, "ALGORITMO DE ASIGNACION DESCONOCIDO");
    }

}

void end_program()
{
    log_destroy(log_memoria);
    config_destroy(memoria_config_file);
    liberar_conexion(socket_servidor_memoria);

}

//comienza comunicacion.c juanpi
void devolver_tabla_inicial(int socket) {
    //cod_op
    int cod = TABLA_SEGMENTOS;

    uint32_t size = sizeof(int) + sizeof(t_ent_ts) * memoria_config.cant_segmentos + sizeof(uint32_t);
    void* buffer = malloc(size);

    memcpy(buffer, &cod, sizeof(int));
    memcpy(buffer + sizeof(int), &memoria_config.cant_segmentos, sizeof(uint32_t));

    void* tabla = crear_tabla_segmentos();

    memcpy(buffer + sizeof(int) + sizeof(uint32_t), tabla, sizeof(t_ent_ts) * memoria_config.cant_segmentos);

    send(socket, buffer, size, NULL);

    free(buffer);
    free(tabla);


}

void devolver_resultado_creacion(op_code resultado, int socket, uint32_t base) {
    if(resultado == OK){
        enviar_paquete_entero(socket, base, resultado);
    }
    else{
        enviar_CodOp(socket, resultado);
    }
}

void devolver_nuevas_bases(int cliente_socket) {
    uint32_t size = (sizeof(uint32_t) * 3) * list_size(LISTA_GLOBAL_SEGMENTOS) + sizeof(uint32_t);
    void* buffer = malloc(size);
    int desplazamiento = 0;
    
    memcpy(buffer, &size, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    t_segmento_memoria* segmento;
    // for each segmento in LISTA_GLOBAL_SEGMENTOS copy its pid, id and base
    for (int i = 0; i < list_size(LISTA_GLOBAL_SEGMENTOS); i++)
    {   
        segmento = list_get(LISTA_GLOBAL_SEGMENTOS, i);
        memcpy(buffer + desplazamiento, &segmento->pid, sizeof(uint32_t));
        desplazamiento += sizeof(uint32_t);
        memcpy(buffer + desplazamiento, &segmento->id_segmento, sizeof(uint32_t));
        desplazamiento += sizeof(uint32_t);
        memcpy(buffer + desplazamiento, &segmento->direccion_base, sizeof(uint32_t));
        desplazamiento += sizeof(uint32_t);
    }

    send(cliente_socket, buffer, size, NULL);
    free(buffer);
}
//no lo usamos
// void procesar_conexion(void *void_args)
// {
//     t_conexion *args = (t_conexion *)void_args;
//     t_log *logger = args->log;
//     int cliente_socket = args->socket;
//     free(args);

//     int cop;

//     while (cliente_socket != -1)
//     {
//         cop = recibir_operacion(cliente_socket);

//         switch (cop)
//         {
//         case CREATE_SEGTABLE:
//             // pthread_mutex_lock(&mutex_memoria);
//             // uint32_t pid;
//             // recv(cliente_socket, &pid, sizeof(uint32_t), NULL);
//             // devolver_tabla_inicial(cliente_socket);
//             // log_info(log_memoria, "Creacion de Proceso PID: %d", pid);
//             // pthread_mutex_unlock(&mutex_memoria);
            

//             break;
//         case MEMORIA_CREATE_SEGMENT:
            
//             t_3_enteros* estructura_create_seg = recibir_3_enteros(cliente_socket);
//             pthread_mutex_lock(&mutex_memoria);
//             uint32_t pid_create_segment;
//             uint32_t id_seg;
//             uint32_t tam_seg;
//             recv(cliente_socket, &pid_create_segment, sizeof(uint32_t), NULL);
//             recv(cliente_socket, &id_seg, sizeof(uint32_t), NULL);
//             recv(cliente_socket, &tam_seg, sizeof(uint32_t), NULL);

//             uint32_t n_base;
//             cod_op_kernel resultado = crear_segmento_memoria(tam_seg, &n_base);
//             // MEMORIA_SEGMENTO_CREADO | MEMORIA_NECESITA_COMPACTACION | EXIT_OUT_OF_MEMORY
//             if (resultado == MEMORIA_SEGMENTO_CREADO)
//             {
//                 t_segmento_memoria* n_seg = malloc(sizeof(t_segmento));
//                 n_seg->pid = pid_create_segment;
//                 n_seg->id_segmento= id_seg;
//                 n_seg->direccion_base = n_base;
//                 n_seg->tamanio_segmento = tam_seg;
//                 list_add_sorted(LISTA_GLOBAL_SEGMENTOS, n_seg, comparador_base_segmento);
//                 log_info(log_memoria, "PID: %d - Crear Segmento: %d - Base: %d - TAMAÑO: %d", pid_create_segment, id_seg, n_base, tam_seg);
//             }
            
//             // print_lista_segmentos();
//             // print_lista_esp(LISTA_ESPACIOS_LIBRES);

//             devolver_resultado_creacion(resultado, cliente_socket, n_base);
//             pthread_mutex_unlock(&mutex_memoria);
//             break;
//         case MEMORIA_FREE_SEGMENT:
//             pthread_mutex_lock(&mutex_memoria);
//             uint32_t pid_free_segment;
//             uint32_t free_seg_id;
//             uint32_t base;
//             uint32_t tam;

//             recv(cliente_socket, &pid_free_segment, sizeof(uint32_t), NULL);
//             recv(cliente_socket, &free_seg_id, sizeof(uint32_t), NULL);
//             recv(cliente_socket, &base, sizeof(uint32_t), NULL);
//             recv(cliente_socket, &tam, sizeof(uint32_t), NULL);

//             borrar_segmento(base, tam);
//             log_info(log_memoria, "PID: %d - Eliminar Segmento: %d - Base: %d - TAMAÑO: %d", pid_free_segment, free_seg_id, base, tam);
//             print_lista_esp(LISTA_ESPACIOS_LIBRES); //
//             pthread_mutex_unlock(&mutex_memoria);
//             break;
//         case MEMORIA_MOV_OUT:
//             pthread_mutex_lock(&mutex_memoria);
//             uint32_t pid_mov_out;
//             uint32_t dir_fisica;
//             uint32_t tam_escrito;
//             recv(cliente_socket, &pid_mov_out, sizeof(uint32_t), NULL);
//             recv(cliente_socket, &dir_fisica, sizeof(uint32_t), NULL);
//             recv(cliente_socket, &tam_escrito, sizeof(uint32_t), NULL);
//             char* valor = malloc(tam_escrito);
//             recv(cliente_socket, valor, tam_escrito, NULL);
            
//             // Para probar
//             // char* cadena = imprimir_cadena(valor, tam_escrito);
//             // printf("Valor recibido: %s\n", cadena);

//             escribir(dir_fisica, valor, tam_escrito);
//             log_info(log_memoria, "PID: %d - Acción: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: CPU", pid_mov_out, dir_fisica, tam_escrito);
//             sleep(memoria_config.retardo_memoria/1000);
//             free(valor);
//             uint32_t mov_out_ok = 1;
//             send(cliente_socket, &mov_out_ok, sizeof(uint32_t), NULL);
//             //char* cosita = leer(dir_fisica, tam_escrito);
//             pthread_mutex_unlock(&mutex_memoria);
//             break;
//         case MEMORIA_MOV_IN:
//             pthread_mutex_lock(&mutex_memoria);
//             uint32_t pid_mov_in;
//             uint32_t dir_fisica_in;
//             uint32_t tam_a_leer;
//             recv(cliente_socket, &pid_mov_in, sizeof(uint32_t), NULL);
//             recv(cliente_socket, &dir_fisica_in, sizeof(uint32_t), NULL);
//             recv(cliente_socket, &tam_a_leer, sizeof(uint32_t), NULL);
//             char* valor_in = leer(dir_fisica_in, tam_a_leer);
//             send(cliente_socket, valor_in, tam_a_leer, NULL);
//             free(valor_in);
//             log_info(log_memoria, "PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: CPU", pid_mov_in, dir_fisica_in, tam_a_leer);
//             sleep(memoria_config.retardo_memoria/1000);
//             pthread_mutex_unlock(&mutex_memoria);
//             break;

//         case COMPACTAR:
//             pthread_mutex_lock(&mutex_memoria);
//             compactar();
//             for(int i = 0; i < list_size(LISTA_GLOBAL_SEGMENTOS); i++)
//             {
//                 t_segmento_memoria* segmento = list_get(LISTA_GLOBAL_SEGMENTOS, i);
//                 log_info(log_memoria, "PID: %d - Segmento: %d - Base: %d - Tamaño: %d", segmento->pid, segmento->id_segmento, segmento->direccion_base, segmento->tamanio_segmento);
//             }
//             devolver_nuevas_bases(cliente_socket);
//             print_lista_esp(LISTA_ESPACIOS_LIBRES);
//             pthread_mutex_unlock(&mutex_memoria);
//             break;
        
//         case LEER_ARCHIVO:
//             pthread_mutex_lock(&mutex_memoria);
//             uint32_t pid_leer_archivo;
//             uint32_t dir_fisica_leer_archivo;
//             uint32_t tam_a_leer_archivo;
//             recv(cliente_socket, &pid_leer_archivo, sizeof(uint32_t), NULL);
//             recv(cliente_socket, &dir_fisica_leer_archivo, sizeof(uint32_t), NULL);
//             recv(cliente_socket, &tam_a_leer_archivo, sizeof(uint32_t), NULL);
//             char* valor_leer_archivo = malloc(tam_a_leer_archivo);
//             recv(cliente_socket, valor_leer_archivo, tam_a_leer_archivo, NULL);
//             escribir(dir_fisica_leer_archivo, valor_leer_archivo, tam_a_leer_archivo);
//             free(valor_leer_archivo);
//             log_info(log_memoria, "PID: %d - Accion: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: FS", pid_leer_archivo, dir_fisica_leer_archivo, tam_a_leer_archivo);
//             sleep(memoria_config.retardo_memoria/1000);
//             uint32_t escritura_ok = 0;
//             send(cliente_socket, &escritura_ok, sizeof(uint32_t), NULL);
//             pthread_mutex_unlock(&mutex_memoria);
//             break;

//         case ESCRIBIR_ARCHIVO:
//             pthread_mutex_lock(&mutex_memoria);
//             uint32_t pid_escribir_archivo;
//             uint32_t dir_fisica_escribir_archivo;
//             uint32_t tam_a_escribir_archivo;
//             recv(cliente_socket, &pid_escribir_archivo, sizeof(uint32_t), NULL);
//             recv(cliente_socket, &dir_fisica_escribir_archivo, sizeof(uint32_t), NULL);
//             recv(cliente_socket, &tam_a_escribir_archivo, sizeof(uint32_t), NULL);
//             char* valor_escribir_archivo = leer(dir_fisica_escribir_archivo, tam_a_escribir_archivo);
//             log_info(log_memoria, "PID: %d - Accion: LEER - Dirección física: %d - Tamaño: %d - Origen: FS", pid_escribir_archivo, dir_fisica_escribir_archivo, tam_a_escribir_archivo);
//             sleep(memoria_config.retardo_memoria/1000);
//             send(cliente_socket, valor_escribir_archivo, tam_a_escribir_archivo, NULL);
//             free(valor_escribir_archivo);
//             pthread_mutex_unlock(&mutex_memoria);
//             break;
//         default:
//             log_error(logger, "Algo anduvo mal en el server Memoria");
//             log_info(logger, "Cop: %d", cop);
//             return;
        
//         }
//     }

//     log_warning(logger, "El cliente se desconectó del server");
//     return;
// }
//termina comunicacion.c juanpi

//comienza utils juanpi
void levantar_estructuras_administrativas() {
    ESPACIO_USUARIO = malloc(memoria_config.tam_memoria);
    ESPACIO_LIBRE_TOTAL = memoria_config.tam_memoria;

    LISTA_ESPACIOS_LIBRES = list_create();
    LISTA_GLOBAL_SEGMENTOS = list_create();

    t_esp* espacio_inicial = malloc(sizeof(t_esp));
    espacio_inicial->base = 0;
    espacio_inicial->limite = memoria_config.tam_memoria;

    list_add(LISTA_ESPACIOS_LIBRES, espacio_inicial);

    crear_segmento_0();
}

void crear_segmento_0() {
    t_esp* espacio = list_get(LISTA_ESPACIOS_LIBRES, 0);
    espacio->base += memoria_config.tam_segmento_0;
    espacio->limite -= memoria_config.tam_segmento_0;
    ESPACIO_LIBRE_TOTAL -= memoria_config.tam_segmento_0;
}

bool comparador_base(void* data1, void* data2) {
    t_esp* esp1 = (t_esp*)data1;
    t_esp* esp2 = (t_esp*)data2;

    return esp1->base < esp2->base;
}

bool comparador_base_segmento(void* data1, void* data2) {
    t_segmento_memoria* seg1 = (t_segmento_memoria*)data1;
    t_segmento_memoria* seg2 = (t_segmento_memoria*)data2;

    return seg1->direccion_base < seg2->direccion_base;
}

void print_lista_esp(t_list* lista) {
    printf("Lista de espacios libres:\n");
    for (int i = 0; i < list_size(lista); i++) {
        t_esp* elemento = list_get(lista, i);
        printf("Elemento %d: base=%u, limite=%u\n", i+1, elemento->base, elemento->limite);
    }
}

void print_lista_segmentos() {
    printf("Lista de segmentos:\n");
    for (int i = 0; i < list_size(LISTA_GLOBAL_SEGMENTOS); i++) {
        t_segmento_memoria* elemento = list_get(LISTA_GLOBAL_SEGMENTOS, i);
        printf("PID %u: ID=%u, BASE=%u, LIMITE=%u\n", elemento->pid, elemento->id_segmento, elemento->direccion_base, elemento->tamanio_segmento);
    }
}

void* crear_tabla_segmentos() {
    void* buffer = malloc(sizeof(t_ent_ts) * memoria_config.cant_segmentos);
    uint32_t despl = 0;
    uint32_t cero = 0;
    uint32_t i = 0;
    uint8_t estado_inicial = 1;

    memcpy(buffer + despl,&i, sizeof(uint32_t)); // ID
    despl+=sizeof(uint32_t);
    i++;

    memcpy(buffer + despl,&cero, sizeof(uint32_t)); // BASE
    despl+=sizeof(uint32_t);

    memcpy(buffer + despl,&memoria_config.tam_segmento_0, sizeof(uint32_t)); // LIMITE
    despl+=sizeof(uint32_t);

    memcpy(buffer + despl, &estado_inicial, sizeof(uint8_t));
    despl+=sizeof(uint8_t);

    estado_inicial = 0;

    for (; i < memoria_config.cant_segmentos; i++)
    {
        memcpy(buffer + despl,&i, sizeof(uint32_t)); // ID
        despl+=sizeof(uint32_t);

        memcpy(buffer + despl,&cero, sizeof(uint32_t)); // BASE
        despl+=sizeof(uint32_t);

        memcpy(buffer + despl,&cero, sizeof(uint32_t)); // LIMITE
        despl+=sizeof(uint32_t);

        memcpy(buffer + despl, &estado_inicial, sizeof(uint8_t));
        despl+=sizeof(uint8_t);
    }
    
    return buffer;
}

int buscar_espacio_libre(uint32_t tam) {
    t_esp* esp;
    t_esp* esp_i;
    switch (memoria_config.algoritmo_asignacion)
    {
    case FIRST:
        for (int i = 0; i < list_size(LISTA_ESPACIOS_LIBRES); i++)
        {   
            esp = list_get(LISTA_ESPACIOS_LIBRES, i);
            if (esp->limite >= tam)
            {
                return i;
            }
        }

        log_info(log_memoria, "NO SE ENCONTRO UN ESPACIO LIBRE, SE NECESITA COMPACTAR");
        return -1;
   
        break;
    
    case WORST:
        esp = list_get(LISTA_ESPACIOS_LIBRES, 0);
        int index_worst = 0;
        for (int i = 1; i < list_size(LISTA_ESPACIOS_LIBRES); i++)
        {   
            esp_i = list_get(LISTA_ESPACIOS_LIBRES, i);
            if (esp_i->limite > esp->limite)
            {
                esp = esp_i;
                index_worst = i;
            }
        }

        if (esp->limite >= tam)
        {
            return index_worst;
        } 

        log_info(log_memoria, "NO SE ENCONTRO UN ESPACIO LIBRE, SE NECESITA COMPACTAR");
        return -1;

        break;
    
    case BEST:
        esp = list_get(LISTA_ESPACIOS_LIBRES, 0);
        int index_best = 0;
        for (int i = 1; i < list_size(LISTA_ESPACIOS_LIBRES); i++)
        {   
            esp_i = list_get(LISTA_ESPACIOS_LIBRES, i);
            if (esp_i->limite >= tam)
            {
                if (esp->limite < tam)
                {
                    esp = esp_i;
                    index_best = i;
                }
                
            }
        }

        if (esp->limite >= tam)
        {
            return index_best;
        } 

        log_info(log_memoria, "NO SE ENCONTRO UN ESPACIO LIBRE, SE NECESITA COMPACTAR");
        return -1;

        break;
    
    default:
        log_error(log_memoria,"ALGO BUSQUEDA ESPACIO DESCONOCIDO");
    }
    
    return -1;
}

op_code crear_segmento_memoria(uint32_t tam, uint32_t* base_resultante) {
    //printf("%d, %d\n", ESPACIO_LIBRE_TOTAL, tam);
    if (ESPACIO_LIBRE_TOTAL < tam)
    {
        log_info(log_memoria, "NO HAY ESPACIO SUFICIENTE PARA CREAR ESE SEGMENTO");
        return OUT_OF_MEMORY;
        // Retornar codop indicando que no hay espacio suficiente.
    }
    
    
    int i_espacio = buscar_espacio_libre(tam);

    if (i_espacio == -1)
    {
        // Retornar codop indicando que es necesario compactar.
        return NECESITO_COMPACTAR;
    }

    t_esp* espacio = list_get(LISTA_ESPACIOS_LIBRES, i_espacio);
    memcpy(base_resultante, &espacio->base, sizeof(uint32_t));

    espacio->base += tam;
    espacio->limite -= tam;
    ESPACIO_LIBRE_TOTAL -= tam;

    if (espacio->limite == 0)
    {
        list_remove(LISTA_ESPACIOS_LIBRES, i_espacio);
        free(espacio);
    }

    return OK;
    
}

bool son_contiguos(t_esp* esp1, t_esp* esp2) {
    return esp1 ->base + esp1->limite == esp2 ->base;
}

int buscar_segmento_por_base(uint32_t base) {
    t_segmento_memoria* segmento;
    for (int i = 0; i < list_size(LISTA_GLOBAL_SEGMENTOS); i++)
    {
        segmento = list_get(LISTA_GLOBAL_SEGMENTOS, i);
        if (segmento->direccion_base == base)
        {
            return i;
        }
    }
    return -1;
}

void borrar_segmento(uint32_t base, uint32_t limite) {
    ESPACIO_LIBRE_TOTAL += limite;

    t_esp* nuevo_esp = malloc(sizeof(t_esp));
    nuevo_esp->base = base;
    nuevo_esp->limite = limite;
    int n_indice = list_add_sorted(LISTA_ESPACIOS_LIBRES, nuevo_esp, comparador_base);
    
    //consolidacion

    if (list_size(LISTA_ESPACIOS_LIBRES) > 1)
    {
        if (n_indice < list_size(LISTA_ESPACIOS_LIBRES) - 1)
        {
            t_esp* posible_espacio_contiguo_abajo = list_get(LISTA_ESPACIOS_LIBRES, n_indice + 1);

            if (son_contiguos(nuevo_esp, posible_espacio_contiguo_abajo))
            {
                nuevo_esp -> limite += posible_espacio_contiguo_abajo->limite;
                list_remove(LISTA_ESPACIOS_LIBRES, n_indice + 1);
                free(posible_espacio_contiguo_abajo);
            }
        }
        


        if (n_indice > 0)
        {
            t_esp* posible_espacio_contiguo_arriba = list_get(LISTA_ESPACIOS_LIBRES, n_indice - 1);

            if (son_contiguos(posible_espacio_contiguo_arriba, nuevo_esp))
            {
                posible_espacio_contiguo_arriba -> limite += nuevo_esp->limite;
                list_remove(LISTA_ESPACIOS_LIBRES, n_indice);
                free(nuevo_esp);
            }
        }
    }
    
    int indice_segmento = buscar_segmento_por_base(base);

    t_segmento_memoria* segmento = list_remove(LISTA_GLOBAL_SEGMENTOS,indice_segmento );
    free(segmento);
    
}

void escribir(uint32_t dir_fisca, void* data, uint32_t size) {
    memcpy(ESPACIO_USUARIO + dir_fisca, data, size);
}

char* leer(uint32_t dir_fisca , uint32_t size) {
    void* data = malloc(size);
    memcpy(data, ESPACIO_USUARIO + dir_fisca, size);
    return data;
}

void compactar() {
    for (int i = 0; i < list_size(LISTA_GLOBAL_SEGMENTOS); i++)
    {
        t_segmento_memoria* segmento = list_get(LISTA_GLOBAL_SEGMENTOS, i);
        t_esp* primer_espacio_libre = list_get(LISTA_ESPACIOS_LIBRES, 0);
        if (segmento->direccion_base > primer_espacio_libre->base )
        {
            memcpy(ESPACIO_USUARIO + primer_espacio_libre->base, ESPACIO_USUARIO + segmento->direccion_base, segmento->tamanio_segmento);
            segmento->direccion_base = primer_espacio_libre->base;
            primer_espacio_libre->base += segmento->tamanio_segmento;
            
            //consolidacion
            if (list_size(LISTA_ESPACIOS_LIBRES) > 1)
            {
                t_esp* posible_espacio_contiguo_abajo = list_get(LISTA_ESPACIOS_LIBRES, 1);

                if (son_contiguos(primer_espacio_libre, posible_espacio_contiguo_abajo))
                {
                    primer_espacio_libre -> limite += posible_espacio_contiguo_abajo->limite;
                    list_remove(LISTA_ESPACIOS_LIBRES, 1);
                    free(posible_espacio_contiguo_abajo);
                }
            }
        }
        
    }

    sleep(memoria_config.retardo_compactacion/1000);
}

//termina utils juanpi


//manejo de conexiones

void recibir_kernel(int SOCKET_CLIENTE_KERNEL)
{

    enviar_mensaje("recibido kernel", SOCKET_CLIENTE_KERNEL);

    int codigoOP = 0;
    while (codigoOP != -1)
    {
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_KERNEL);
        switch (codigoOperacion)
        {
        case INICIAR_ESTRUCTURAS:
            log_trace(log_memoria, "recibi el op_cod %d INICIAR_ESTRUCTURAS", codigoOperacion);
            //- Creación de Proceso: “Creación de Proceso PID: <PID>”
            pthread_mutex_lock(&mutex_memoria);
            uint32_t pid = recibir_entero_u32(SOCKET_CLIENTE_KERNEL, log_memoria);
            log_info(log_memoria, "Creacion de Proceso PID: %d", pid);
            devolver_tabla_inicial(SOCKET_CLIENTE_KERNEL);
            pthread_mutex_unlock(&mutex_memoria);
            break;
        break;
        
        case DELETE_PROCESS:
        // juanpi borra todos los segmentos que tengan ese PID => ej: hace deletet_segment 10
        uint32_t pid_delete = recibir_entero_u32(SOCKET_CLIENTE_KERNEL, log_memoria);
        //borrar_proceso(pid_delete);
        break;
        case CREATE_SEGMENT:
            pthread_mutex_lock(&mutex_memoria);
            t_3_enteros* create_data = recibir_3_enteros(SOCKET_CLIENTE_KERNEL);
            uint32_t pid_create_segment = create_data->entero1;
            uint32_t id_seg = create_data->entero2;
            uint32_t tam_seg = create_data->entero3;

            uint32_t n_base;
            uint32_t resultado = crear_segmento_memoria(tam_seg, &n_base);
            
            if (resultado == OK)
            {
                t_segmento_memoria* n_seg = malloc(sizeof(t_segmento_memoria));
                n_seg->pid = pid_create_segment;
                n_seg->id_segmento= id_seg;
                n_seg->direccion_base = n_base;
                n_seg->tamanio_segmento = tam_seg;
                list_add_sorted(LISTA_GLOBAL_SEGMENTOS, n_seg, comparador_base_segmento);
                log_info(log_memoria, "PID: %d - Crear Segmento: %d - Base: %d - TAMAÑO: %d", pid_create_segment, id_seg, n_base, tam_seg);
            }

            devolver_resultado_creacion(resultado, SOCKET_CLIENTE_KERNEL, n_base);
            pthread_mutex_unlock(&mutex_memoria);
            break;

        case DELETE_SEGMENT:
            // Debe recibir el id del segmento que desea eliminar
            pthread_mutex_lock(&mutex_memoria);
            t_4_enteros* delete_data = recibir_4_enteros(SOCKET_CLIENTE_KERNEL);
            uint32_t pid_free_segment = delete_data->entero1;
            uint32_t free_seg_id = delete_data->entero2;
            uint32_t base = delete_data->entero3;
            uint32_t tam = delete_data->entero4;

            borrar_segmento(base, tam);
            log_info(log_memoria, "PID: %d - Eliminar Segmento: %d - Base: %d - TAMAÑO: %d", pid_free_segment, free_seg_id, base, tam);
            print_lista_esp(LISTA_ESPACIOS_LIBRES); //
            pthread_mutex_unlock(&mutex_memoria);
            break;

         case COMPACTAR:
            pthread_mutex_lock(&mutex_memoria);
            compactar();
            for(int i = 0; i < list_size(LISTA_GLOBAL_SEGMENTOS); i++)
            {
                t_segmento_memoria* segmento = list_get(LISTA_GLOBAL_SEGMENTOS, i);
                log_info(log_memoria, "PID: %d - Segmento: %d - Base: %d - Tamaño: %d", segmento->pid, segmento->id_segmento, segmento->direccion_base, segmento->tamanio_segmento);
            }
            devolver_nuevas_bases(SOCKET_CLIENTE_KERNEL);
            print_lista_esp(LISTA_ESPACIOS_LIBRES);
            pthread_mutex_unlock(&mutex_memoria);
            break;

        case -1:
            codigoOP = codigoOperacion;
            break;
        // se desconecta kernel
        default:
            log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(log_memoria, "se desconecto kernel");
    sem_post(&finModulo);
}

void recibir_cpu(int SOCKET_CLIENTE_CPU)
{

    enviar_mensaje("recibido cpu", SOCKET_CLIENTE_CPU);
    log_trace(log_memoria, "recibido cpu");
    int codigoOP = 0;
    while (codigoOP != -1)
    {
        log_warning(log_memoria,"me quedo esperando");
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_CPU);
        //sleep(memoria_config.retardo_memoria);
        switch (codigoOperacion)
        {
        case MENSAJE:
            log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
            break;

        case MOV_IN: 
            pthread_mutex_lock(&mutex_memoria);
            t_3_enteros* mov_in_data = recibir_3_enteros(SOCKET_CLIENTE_CPU);
            uint32_t pid_mov_in = mov_in_data->entero1;
            uint32_t dir_fisica_in = mov_in_data->entero2;
            uint32_t tam_a_leer = mov_in_data->entero3;
            char* valor_in = leer(dir_fisica_in, tam_a_leer);
            
            //send(cliente_socket, valor_in, tam_a_leer, NULL);
            
            sleep(memoria_config.retardo_memoria/1000);
            enviar_paquete_string(SOCKET_CLIENTE_CPU, valor_in, MOV_IN_OK, tam_a_leer);
            free(valor_in);
            log_info(log_memoria, "PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: CPU", pid_mov_in, dir_fisica_in, tam_a_leer);
            pthread_mutex_unlock(&mutex_memoria);
            break;


        case MOV_OUT: //(Dirección Fisica, Registro): Lee el valor del Registro y lo escribe en la dirección física de memoria obtenida a partir de la Dirección Lógica.
            pthread_mutex_lock(&mutex_memoria);
            t_3_enteros* mov_out_data = recibir_3_enteros(SOCKET_CLIENTE_CPU);
            uint32_t pid_mov_out = mov_out_data->entero1;
            uint32_t dir_fisica = mov_out_data->entero2;
            uint32_t tam_escrito = mov_out_data->entero3;
            char* valor = malloc(tam_escrito);
            recv(SOCKET_CLIENTE_CPU, valor, tam_escrito, NULL);
            // Para probar
            // char* cadena = imprimir_cadena(valor, tam_escrito);
            // printf("Valor recibido: %s\n", cadena);

            escribir(dir_fisica, valor, tam_escrito);
            log_info(log_memoria, "PID: %d - Acción: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: CPU", pid_mov_out, dir_fisica, tam_escrito);
            free(valor);
            enviar_CodOp(SOCKET_CLIENTE_CPU, MOV_OUT_OK);
            sleep(memoria_config.retardo_memoria/1000);
            pthread_mutex_unlock(&mutex_memoria);
            break;

        case -1:
            codigoOP = codigoOperacion;
        break;
        
        default:
             log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(log_memoria, "se desconecto CPU");
}

void recibir_fileSystem(int SOCKET_CLIENTE_FILESYSTEM)
{

    enviar_mensaje("recibido fileSystem", SOCKET_CLIENTE_FILESYSTEM);
    int codigoOP = 0;
    while (codigoOP != -1)
    {
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_FILESYSTEM);
        log_warning(log_memoria, "llegue a fs");
        switch (codigoOperacion)
        {
        case MENSAJE:
            
            log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);

            break;

        case F_READ:

        pthread_mutex_lock(&mutex_memoria);
            t_string_3enteros* fread_data = recibir_string_3enteros(SOCKET_CLIENTE_FILESYSTEM);
            uint32_t pid_leer_archivo = fread_data->entero1;
            uint32_t dir_fisica_leer_archivo = fread_data->entero2;
            uint32_t tam_a_leer_archivo = fread_data->entero3;
            char* valor_leer_archivo = fread_data->string;
            //recv(cliente_socket, &pid_leer_archivo, sizeof(uint32_t), NULL);
            //recv(cliente_socket, &dir_fisica_leer_archivo, sizeof(uint32_t), NULL);
            //recv(cliente_socket, &tam_a_leer_archivo, sizeof(uint32_t), NULL);
            //recv(cliente_socket, valor_leer_archivo, tam_a_leer_archivo, NULL);
            escribir(dir_fisica_leer_archivo, valor_leer_archivo, tam_a_leer_archivo);
            free(valor_leer_archivo);
            log_info(log_memoria, "PID: %d - Accion: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: FS", pid_leer_archivo, dir_fisica_leer_archivo, tam_a_leer_archivo);
            sleep(memoria_config.retardo_memoria/1000);
            enviar_CodOp(SOCKET_CLIENTE_FILESYSTEM,F_READ_OK);
        pthread_mutex_unlock(&mutex_memoria);
            break;
        
        case F_WRITE:
        pthread_mutex_lock(&mutex_memoria);
            t_3_enteros* fwrite_data = recibir_3_enteros(SOCKET_CLIENTE_FILESYSTEM);
            uint32_t pid_escribir_archivo = fwrite_data->entero1;
            uint32_t dir_fisica_escribir_archivo = fwrite_data->entero2;
            uint32_t tam_a_escribir_archivo = fwrite_data -> entero3;

            char* valor_escribir_archivo = leer(dir_fisica_escribir_archivo, tam_a_escribir_archivo);
            log_info(log_memoria, "PID: %d - Accion: LEER - Dirección física: %d - Tamaño: %d - Origen: FS", pid_escribir_archivo, dir_fisica_escribir_archivo, tam_a_escribir_archivo);
            sleep(memoria_config.retardo_memoria/1000);
            enviar_paquete_string(SOCKET_CLIENTE_FILESYSTEM, valor_escribir_archivo, sizeof(valor_escribir_archivo), F_WRITE_OK);
            free(valor_escribir_archivo);
        pthread_mutex_unlock(&mutex_memoria);
            break;

        case -1:
        codigoOP = codigoOperacion;
        
        break;

        default:
            log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(log_memoria, "se desconecto FILESYSTEM");
}


/*memoriaViejo
//KERNEL
void recibir_kernel(int SOCKET_CLIENTE_KERNEL)
{

    enviar_mensaje("recibido kernel", SOCKET_CLIENTE_KERNEL);
    //int cliente_socket = SOCKET_CLIENTE_KERNEL;
    int codigoOP = 0;
    while (codigoOP != -1)
    {
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_KERNEL);
        switch (codigoOperacion)
        {
        case INICIAR_ESTRUCTURAS:
            int id_inicio_estructura = recibir_entero(SOCKET_CLIENTE_KERNEL, log_memoria);
            t_proceso* nuevo_proceso = crear_proceso_en_memoria(id_inicio_estructura);
            log_trace(log_memoria, "recibi el op_cod %d INICIAR_ESTRUCTURAS", codigoOperacion);
            log_trace(log_memoria, "creando paquete con tabla de segmentos base");
            log_warning(log_memoria, "la pos de memoria es %d", MEMORIA_PRINCIPAL);
            // t_list* tabla_adaptada = adaptar_TDP_salida();
            imprimir_tabla_segmentos(((t_proceso*)(list_get(tabla_de_procesos, 0)))->tabla_segmentos, log_memoria);
            //- Creación de Proceso: “Creación de Proceso PID: <PID>”
            // t_proceso* proceso_adaptado = buscar_proceso_aux(id_inicio_estructura, tabla_adaptada);
            enviar_tabla_segmentos(SOCKET_CLIENTE_KERNEL, TABLA_SEGMENTOS, nuevo_proceso);//tabla adaptada
            log_trace(log_memoria, "envio tabla de segmentos base");
        break;
        
        case DELETE_PROCESS:
        int PID = recibir_entero(SOCKET_CLIENTE_KERNEL, log_memoria);
        borrar_proceso(PID);
        break;
        case CREATE_SEGMENT:
            pthread_mutex_lock(&mutexUnicaEjecucion);
            t_3_enteros* estructura_3_enteros = recibir_3_enteros(SOCKET_CLIENTE_KERNEL);
            int id_proceso = estructura_3_enteros->entero1;
            int id_segmento_nuevo = estructura_3_enteros->entero2;
            int tamanio = estructura_3_enteros->entero3;


            if(puedoGuardar(tamanio)==1){
                t_list* segmentosDisponibles = buscarSegmentosDisponibles();
                log_warning(log_memoria, "segmentos disponibles = %d", list_size(segmentosDisponibles));
                

                bool menorTamanio(t_segmento_memoria* unSegmento){
                    return (unSegmento->tamanio_segmento < tamanio);
                }
                t_list* segmentosDisponibles2 = list_filter(segmentosDisponibles, menorTamanio);
                log_info(log_memoria, "la cantidad de segmentos disponibles de menor tamaño al tamanio del segmento es %d", list_size(segmentosDisponibles2));
                if(list_is_empty(segmentosDisponibles2)==0){//chequear que los espacios son de menor tamaño que el tamanio del segmento
                    log_warning(log_memoria, "necesito compactar");
                    enviar_CodOp(SOCKET_CLIENTE_KERNEL, NECESITO_COMPACTAR);
                }
                else{
                //busco un espacio segun el algoritmo de ordenamiento
                t_segmento* segmento_nuevo = buscarSegmentoSegunTamanio(tamanio);
                segmento_nuevo->id_segmento=id_segmento_nuevo;
                t_proceso* proceso_con_nuevo_segmento = buscar_proceso(id_proceso);
                list_add(proceso_con_nuevo_segmento->tabla_segmentos, segmento_nuevo);
                log_warning(log_memoria,"Creación de Segmento: PID: %d - Crear Segmento: %d - Base: %d - TAMAÑO: %d", id_proceso, id_segmento_nuevo, segmento_nuevo->direccion_base, segmento_nuevo->tamanio_segmento);
                //list_add(proceso_con_nuevo_segmento->tabla_segmentos, segmento_nuevo);
                ocuparBitMap(segmento_nuevo->direccion_base, segmento_nuevo->tamanio_segmento);
                enviar_paquete_entero(SOCKET_CLIENTE_KERNEL, segmento_nuevo->direccion_base, OK);
                
                log_warning(log_memoria,"envie paquete con entero %d",segmento_nuevo->direccion_base);
                }
            }
            else{
            log_warning(log_memoria, "sin memoria");
            enviar_CodOp(SOCKET_CLIENTE_KERNEL, OUT_OF_MEMORY);
            }
            pthread_mutex_unlock(&mutexUnicaEjecucion);
            break;

        case DELETE_SEGMENT:
            // Debe recibir el id del segmento que desea eliminar
            pthread_mutex_lock(&mutexUnicaEjecucion);
            t_2_enteros* data_delete = recibir_2_enteros(SOCKET_CLIENTE_KERNEL);
            int id_proceso_delete = data_delete->entero1;
            int id_segmento_delete = data_delete->entero2;
            t_proceso* proceso_sin_segmento = borrar_segmento(id_proceso_delete,id_segmento_delete);
            enviar_tabla_segmentos(SOCKET_CLIENTE_KERNEL, TABLA_SEGMENTOS, proceso_sin_segmento);
            pthread_mutex_unlock(&mutexUnicaEjecucion);
            break;


         case COMPACTAR:
            pthread_mutex_lock(&mutexUnicaEjecucion);
             //sleep(memoria_config.retardo_compactacion);
             log_warning(log_memoria, "Solicitud de Compactación");
             compactacion();
             log_warning(log_memoria,"sali de compactacion");
             imprimir_tabla_segmentos(((t_proceso*)(list_get(tabla_de_procesos, 0)))->tabla_segmentos, log_memoria);// pero si me llegan en orden xd
             enviar_todas_tablas_segmentos(SOCKET_CLIENTE_KERNEL, tabla_de_procesos, OK_COMPACTACION, log_memoria);
             pthread_mutex_unlock(&mutexUnicaEjecucion);
             break;

        case -1:
            codigoOP = codigoOperacion;
            break;
        // se desconecta kernel
        default:
            log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(log_memoria, "se desconecto kernel");
    sem_post(&finModulo);
}
//CPU
void recibir_cpu(int SOCKET_CLIENTE_CPU)
{

    enviar_mensaje("recibido cpu", SOCKET_CLIENTE_CPU);
    log_trace(log_memoria, "recibido cpu");
    int codigoOP = 0;
    while (codigoOP != -1)
    {
        log_warning(log_memoria,"me quedo esperando");
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_CPU);
        //sleep(memoria_config.retardo_memoria);
        switch (codigoOperacion)
        {
        case MENSAJE:
            log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
            break;

        case MOV_IN: 
            pthread_mutex_lock(&mutexUnicaEjecucion);//(Registro, Direc_base ,size): Lee el valor de memoria correspondiente a la Dirección Lógica y lo almacena en el Registro.
            //falta el PID
            //PID|direc_base|size
            //int PID = recibir_entero(SOCKET_CLIENTE_CPU, log_memoria)
            t_3_enteros* movin = recibir_3_enteros(SOCKET_CLIENTE_CPU);
            log_info(log_memoria, "PID: %d - Accion: LEER - Direccion física: %d - Tamaño: %d - Origen: CPU",movin->entero1, movin->entero2,movin->entero3);
            mov_in(SOCKET_CLIENTE_CPU, movin->entero2, movin->entero3);
            pthread_mutex_unlock(&mutexUnicaEjecucion);
            break;

        case MOV_OUT: //(Dirección Fisica, Registro): Lee el valor del Registro y lo escribe en la dirección física de memoria obtenida a partir de la Dirección Lógica.
            pthread_mutex_lock(&mutexUnicaEjecucion);
            recive_mov_out* data_mov_out = recibir_mov_out(SOCKET_CLIENTE_CPU);
            //void * registro = (void*)recibir_string(SOCKET_CLIENTE_CPU, log_memoria);
            int dir_fis = data_mov_out->DF;
            int size = data_mov_out->size;
            void* registro = (void*) data_mov_out->registro;
            log_info(log_memoria,"PID: %d - Acción: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: CPU", data_mov_out->PID, dir_fis, size); 
            ocuparBitMap(dir_fis, size);
            log_info(log_memoria, "ya ocupe bitmap");

            ocuparMemoria(registro, dir_fis, size);
            log_info(log_memoria, "ya ocupe memoria");
            char* aux = string_new();
            memcpy(aux, MEMORIA_PRINCIPAL + dir_fis, size);
            log_info(log_memoria, "Lo que se escribio en memoria es: %s", aux);
            //falta chequear que el tipo que se pide para los size este bien
            enviar_CodOp(SOCKET_CLIENTE_CPU, MOV_OUT_OK);
            log_info(log_memoria, "ya envie codop");
            pthread_mutex_unlock(&mutexUnicaEjecucion);
            break;

        case -1:
            codigoOP = codigoOperacion;
        break;
        
        default:
             log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(log_memoria, "se desconecto CPU");
}
//FILESYSTEM
void recibir_fileSystem(int SOCKET_CLIENTE_FILESYSTEM)
{

    enviar_mensaje("recibido fileSystem", SOCKET_CLIENTE_FILESYSTEM);
    int codigoOP = 0;
    while (codigoOP != -1)
    {
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_FILESYSTEM);
        sleep(memoria_config.retardo_memoria);
        log_warning(log_memoria, "llegue a fs");
        switch (codigoOperacion)
        {
        case MENSAJE:
            
            log_trace(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);

            break;

        case F_READ:
        pthread_mutex_lock(&mutexUnicaEjecucion);
        log_warning(log_memoria, "entre al fread");
        t_string_3enteros* fRead = recibir_string_3enteros(SOCKET_CLIENTE_FILESYSTEM);
        
        uint32_t pid_leer_arhivo = fRead->entero1;
        uint32_t direccion_fisica_leer_archivo = fRead->entero2;
        uint32_t tam_a_leer_archivo = fRead->entero3;
        char* valor_leer_archivo = malloc(tam_a_leer_archivo);
        valor_leer_archivo = fRead->string;
        log_info(log_memoria, "el stream recibido es %d", valor_leer_archivo);
        escribir(direccion_fisica_leer_archivo, valor_leer_archivo, tam_a_leer_archivo);
        
        log_info(log_memoria, "PID: %d - Accion: ESCRIBIR - Direccion física: %d - Tamaño: %d - Origen: FS",fRead->entero1, fRead->entero2,fRead->entero3);

        free(valor_leer_archivo);
        
        sleep(memoria_config.retardo_memoria/1000);
        
        enviar_CodOp(SOCKET_CLIENTE_FILESYSTEM,F_READ_OK);
        pthread_mutex_unlock(&mutexUnicaEjecucion);
            break;
        
        case F_WRITE:
        pthread_mutex_lock(&mutexUnicaEjecucion);
        log_warning(log_memoria, "Entre al fwrite");
        t_3_enteros* fWrite = recibir_3_enteros(SOCKET_CLIENTE_FILESYSTEM);
        uint32_t pid_escribir_arhivo = fWrite->entero1;
        uint32_t direccion_fisica_escribir_archivo = fWrite->entero2;
        uint32_t tam_a_escribir_archivo = fWrite->entero3;

        char* valor_escribir_archivo = leer(direccion_fisica_escribir_archivo, tam_a_escribir_archivo);
        log_trace(log_memoria,"PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: FS", fWrite->entero1, fWrite->entero2, fWrite->entero3); 
        sleep(memoria_config.retardo_memoria/1000);

        enviar_paquete_string(SOCKET_CLIENTE_FILESYSTEM, valor_escribir_archivo,F_WRITE_OK, tam_a_escribir_archivo);
        free(valor_escribir_archivo);
        pthread_mutex_unlock(&mutexUnicaEjecucion);
        break;
        case -1:
        codigoOP = codigoOperacion;
        
        break;

        default:
            log_trace(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(log_memoria, "se desconecto FILESYSTEM");
}

/* LOGS NECESAIROS Y OBLIGATORIOS
//- Acceso a espacio de usuario: “PID: <PID> - Acción: <LEER / ESCRIBIR> - Dirección física: <DIRECCIÓN_FÍSICA> - Tamaño: <TAMAÑO> - Origen: <CPU / FS>”



t_proceso* crear_proceso_en_memoria(int id_proceso){
    t_proceso* nuevoProceso = malloc(sizeof(t_proceso));
    
    nuevoProceso->id = id_proceso;

    generar_tabla_segmentos(nuevoProceso);

    pthread_mutex_lock(&listaProcesos);
    list_add(tabla_de_procesos, nuevoProceso);
    pthread_mutex_unlock(&listaProcesos);

    return nuevoProceso;
}

void generar_tabla_segmentos(t_proceso* proceso){
	t_list* nuevaTabla = list_create();
	agregar_segmento_0(nuevaTabla);
    proceso->tabla_segmentos = nuevaTabla;
}

void agregar_segmento_0(t_list* nueva_tabla_segmentos){

    list_add(nueva_tabla_segmentos, segmento_compartido);

}


void enviar_tabla_segmentos(int conexion, int codOP, t_proceso* proceso) {
	t_paquete* paquete = crear_paquete_op_code(codOP);
	agregar_entero_a_paquete(paquete, list_size(proceso->tabla_segmentos));
	agregar_tabla_a_paquete(paquete, proceso, log_memoria);
	enviar_paquete(paquete, conexion);

	eliminar_paquete(paquete);
}

//------------------------ SERIALIZACION DE LA TABLA CON MOREL CON MOREL -----------------------------

void agregar_tabla_a_paquete(t_paquete *paquete, t_proceso *proceso, t_log *logger)
{
    t_list *tabla_segmentos = proceso->tabla_segmentos;
    int tamanio = list_size(tabla_segmentos);
    // por la cantidad de elementos que hay dentro de la tabla de segmentos
    // serializar cada uno de sus elementos.
    // serializo del primer elemento de la tabla todos sus componentes y lo coloco en un nuevo t_list*
    // mismo con el segundo y asi hasta la long de la lista, luego serializo la nueva lista.

    for (int i = 0; i < tamanio; i++)
    {
        agregar_entero_a_paquete(paquete, (((t_segmento *)list_get(tabla_segmentos, i))->id_segmento));
        agregar_entero_a_paquete(paquete, (((t_segmento *)list_get(tabla_segmentos, i))->direccion_base));
        agregar_entero_a_paquete(paquete, (((t_segmento *)list_get(tabla_segmentos, i))->tamanio_segmento));
    }

    // te mando todos los segmentos de una, vs del otro lado los tomas y los vas metiendo en un t_list
}

//
//  KERNEL
//
void liberar_bitmap_segmento(t_segmento* segmento){
// liberar bitmap
    liberarBitMap(segmento->direccion_base, segmento->tamanio_segmento);
}
void eliminar_tabla_segmentos(t_list* tabla_segmentos)
{
// liberar cada segmento de la tabla
    list_iterate(tabla_segmentos, (void*)liberar_bitmap_segmento);
    for (int i = 0; i < list_size(tabla_segmentos); i++)
    {
        free(list_get(tabla_segmentos, i));
    }
    
}
void eliminar_proceso(t_proceso* proceso){
    // Eliminar la tabla de segmentos
    list_clean_and_destroy_elements(proceso->tabla_segmentos, (void*)eliminar_tabla_segmentos);
}

void borrar_proceso(int PID){
        // Eliminar la instancia de la tabla de procesos => list_remove_and_destroy_by_condition
    bool mismoIdProc(t_proceso* unProceso){
    return (unProceso->id == PID);
    }
    list_remove_and_destroy_by_condition(tabla_de_procesos, mismoIdProc , eliminar_proceso);
    log_warning(log_memoria,"Eliminación de Proceso PID: %d", PID);
}


//CREATE_SEGMENT

//void* crear_segmento(int PID, int size){
//    if()
//}

//DELETE_SEGMENT

t_proceso* buscar_proceso(int id_proceso){
    
    bool mismoIdProc(t_proceso* unProceso){
    return (unProceso->id == id_proceso);
    }

    pthread_mutex_lock(&listaProcesos);
    t_proceso* proceso = list_find(tabla_de_procesos, mismoIdProc);
    pthread_mutex_unlock(&listaProcesos);
    return proceso;
}

t_proceso* buscar_proceso_aux(int id_proceso, t_list* tabla_aux){
    
    bool mismoIdProc(t_proceso* unProceso){
    return (unProceso->id == id_proceso);
    }

    pthread_mutex_lock(&listaProcesos);
    t_proceso* proceso = list_find(tabla_aux, mismoIdProc);
    pthread_mutex_unlock(&listaProcesos);
    return proceso;
}

t_proceso* borrar_segmento(int PID,int id_segmento_elim){
    
    bool mismoIdProc(t_proceso* unProceso){
    return (unProceso->id == PID);
    }
    t_proceso* proceso_del_segmento = list_find(tabla_de_procesos, mismoIdProc);
    bool mismoIdSeg(t_segmento* unSegmento){
        return (unSegmento->id_segmento == id_segmento_elim);
    }
    t_segmento* segmento_a_eliminar = list_find(proceso_del_segmento->tabla_segmentos, mismoIdSeg);
    log_warning(log_memoria,"PID: %d - Eliminar Segmento: %d, Base: %d - TAMAÑO: %d", PID, id_segmento_elim, segmento_a_eliminar->direccion_base, segmento_a_eliminar->tamanio_segmento);
    list_remove_by_condition(proceso_del_segmento->tabla_segmentos, mismoIdSeg);
    liberarBitMap(segmento_a_eliminar->direccion_base,segmento_a_eliminar->tamanio_segmento);
    return proceso_del_segmento;
    
}

//
//  CPU
//

void mov_in(int socket_cliente,int direc_fisica, int size){
    //para guido: antes de enviar la direccion fisica, castearla a void* si es q no esta hecha
    char *registro = string_new();

    memcpy(registro, MEMORIA_PRINCIPAL+direc_fisica ,size);

    registro[size] = '\0';

    log_error(log_memoria, "el valor a enviar es %s", registro);

    enviar_paquete_string(socket_cliente, registro, MOV_IN_OK, strlen(registro)+1);
    //ocuparMemoria(registro, direc_logica, size);
    //ocuparBitMap(direc_logica, size);
}

int puedoGuardar(int quieroGuardar)
{ // RECIBE CANT BYTES QUE QUIERO GUARDAR

    int tamanioLibre = tamanioTotalDisponible();
    log_info(log_memoria, "Hay %d espacio libre, quiero guardar %d", tamanioLibre, quieroGuardar);
    if (quieroGuardar <= tamanioLibre)
    {
        return 1;
    }
    else
        return 0; // DEVUELVE 1 SI HAY ESPACIO SUFICIENTE PARA GUARDAR LO QUE QUIERO GUARDAR
}

int tamanioTotalDisponible(void)
{

    int contador = 0;
    int desplazamiento = 0;

    while (desplazamiento < memoria_config.tam_memoria)
    {

        pthread_mutex_lock(&mutexBitMapSegment);
        if ((bitarray_test_bit(bitMapSegment, desplazamiento) == 0))
        {
            contador++;
        }
        pthread_mutex_unlock(&mutexBitMapSegment);
        desplazamiento++;
    }

    return contador;
}

/*REIVISAR EN CASO DE Q NO FUNCIONEN EL CREATE_SEGMENT


bool necesitoCompactar(int sizeRequerido){
    return  
}
t_list* espaciosLibres(){
    for (int i = 0; i < memoria_config.tam_memoria; i++)
    {
        if(bitarray_test_bit(bitMapSegment, 0){

        }    
    }
    
    
}


// GuardarEnMemoria
t_segmento *guardarElemento(void *elemento, int size)
{

    t_segmento *unSegmento = malloc(sizeof(t_segmento));
    t_segmento *aux;

    aux = buscarSegmentoSegunTamanio(size); // BUSCA UN SEGMENTO LIBRE PARA GUARDAR LAS TAREAS
    guardarEnMemoria(elemento, aux, size);

    unSegmento->id_segmento = aux->id_segmento;
    unSegmento->direccion_base = aux->direccion_base;
    unSegmento->tamanio_segmento = size;

    free(aux);

    return unSegmento; // DEVUELVE EL SEGMENTO QUE FUE GUARDADO
}

t_segmento* buscarSegmentoSegunTamanio(int size){ 

    t_segmento* segmento;
    t_list* todosLosSegLibres;

    todosLosSegLibres =  buscarSegmentosDisponibles(); //PONE TODOS LOS SEGMENTOS VACIOS EN UNA LISTA 

    t_list* segmentosCandidatos;
    segmentosCandidatos = puedenGuardar(todosLosSegLibres , size); //ME DEVUELVE LOS SEGMENTOS QUE EL TIENEN ESPACIO NECESARIO PARA GUARDAR
    //log_info(logger,"Hay %d segmentos candidatos", list_size(segmentosCandidatos));
    if(list_is_empty(segmentosCandidatos)){
        log_error(log_memoria,"No se ha compactado correctamente");
        
    }else if(list_size(segmentosCandidatos)== 1){
        segmento = list_get(segmentosCandidatos, 0);
        segmento->tamanio_segmento = size;
    }else{
        segmento = elegirSegCriterio(segmentosCandidatos, size); //SI EN LA LISTA HAY MAS DE UN SEGMENTO VA A ELEGIR EN QUE SEGMENTO LO VA A GUARDAR SEGUN EL CRITERIO

    }
    
    int mismoIdSeg(t_segmento* unSegmento){
        return (unSegmento->id_segmento == segmento->id_segmento);
    }
    
    list_remove_by_condition(todosLosSegLibres, (void*)mismoIdSeg); //Saco el segmento de la lista asi las puedo eliminar
    
	//list_destroy(todosLosSegLibres);
	//list_destroy(segmentosCandidatos);
	eliminarLista(todosLosSegLibres);
	list_destroy(segmentosCandidatos);
    
    return segmento;
}


t_list* buscarSegmentosDisponibles(){
    
    t_list* segmentos = list_create();
    int base = 0;

    //YA SABEMOS QUE HAY LUGAR, SINO NO HUBIESE     
    
    while(base < (memoria_config.tam_memoria)){
        t_segmento* unSegmento = buscarUnLugarLibre(&base);
        list_add(segmentos,unSegmento);
    }

    return segmentos;    
}

t_segmento* buscarUnLugarLibre(int* base){
    t_segmento* unSegmento = malloc(sizeof(t_segmento));
    int tamanio = 0;
     
    pthread_mutex_lock(&mutexBitMapSegment);
    if(bitarray_test_bit(bitMapSegment, *base) == 1){ //SI EL PRIMERO ES UN UNO, VA A CONTAR CUANDOS ESTAN OCUPADOS DESDE ESE Y CAMBIA LA BASE	
        int desplazamiento = contarEspaciosOcupadosDesde(bitMapSegment, *base); //CUENTA ESPACIOS OCUPADOS DESDE LA ABASE INDICADA
        *base += desplazamiento; //
    }
    pthread_mutex_unlock(&mutexBitMapSegment);
    // ACA YA LA BASE ESTA EN EL PRIMER 0 LIBRE
    tamanio = contarEspaciosLibresDesde(bitMapSegment, *base);  //TAMANIO ES EL TAMANIO DEL SEGMENTO CON 0 LIBRES //ACA MUERE
    
    unSegmento->id_segmento = generarId();// es al pedo 
    unSegmento->direccion_base = *base;
    unSegmento->tamanio_segmento = tamanio;
    
    //MUEVO LA BASE PARA BUSCAR EL SIGUIENTE SEGMENTO DESPUES
    *base +=  tamanio;

    return unSegmento;
}

///  es al pedo ya que la cambio al final de la func

int generarId(){
	
	pthread_mutex_lock(&mutexIdGlobal);
	int t = idGlobal;
	idGlobal++;
	pthread_mutex_unlock(&mutexIdGlobal);
	return t;
}

t_list* puedenGuardar(t_list* segmentos, int size){
   
    t_list* aux;

    int puedoGuardarSeg(t_segmento* segmento){
        return(segmento->tamanio_segmento >= size);
    }
    aux = list_filter(segmentos, (void*)puedoGuardarSeg);
    log_error(log_memoria,"tamaño de la lista aux = %d", list_size(aux));

    return aux;
}

void guardarEnMemoria(void *elemento, t_segmento *segmento, int size)
{

    ocuparBitMap(segmento->direccion_base, size);
    ocuparMemoria(elemento, segmento->direccion_base, size);
}

void ocuparMemoria(void *elemento, int base, int size)
{
    pthread_mutex_lock(&mutexMemoria);
    memcpy(MEMORIA_PRINCIPAL + base, elemento, size);
    pthread_mutex_unlock(&mutexMemoria);
}


//Criterio de asignacio
t_segmento* elegirSegCriterio(t_list* segmentos, int size){

    t_segmento* segmento;
    
    log_info(log_memoria,"Elijo segun %s", memoria_config.algoritmo_asignacion);
    
    imprimir_tabla_segmentos(segmentos, log_memoria);

    if(strcmp(memoria_config.algoritmo_asignacion,"FIRST") == 0){//NO ENTRA LA CONCHA DE SU MADRE
        segmento = list_get(segmentos,0); //FIRST FIT DEVUELVE EL PRIMER SEGMENTO DONDE PUEDE GUARDARSE
    }else if(strcmp(memoria_config.algoritmo_asignacion,"BEST") == 0){
    	log_info(log_memoria,"Entre por BEST");
        segmento = segmentoBestFit(segmentos, size);
    }else if(strcmp(memoria_config.algoritmo_asignacion,"WORST") == 0){
        segmento = segmentoWorstFit(segmentos, size);
    }

    if(segmento->tamanio_segmento>size){
        segmento->tamanio_segmento=size;
    }

    return segmento;
}

t_segmento* segmentoBestFit(t_list* segmentos, int size){

    t_segmento* segmento;
    log_warning(log_memoria, "llega el size: %d", size);//borrar
    int igualTamanio(t_segmento* segmento){
        return(segmento -> tamanio_segmento == size); //SEGMENTO QUE TENGA EL MISMO TAMANIO QUE LO QUE QUIERO GUARDAR
    }
    t_segmento* segmentoDeIgualTamanio = list_find(segmentos, (void*)igualTamanio); //ME DEVUELVE EL SEGMENTO DE MISMO TAMANIO

    if(segmentoDeIgualTamanio != NULL){
        segmento = segmentoDeIgualTamanio;
        log_trace(log_memoria, "hay igual tamaño"); //borrar
    }else{      //SI NO ENCONTRE UN SEGMENTO DEL MISMO TAMANIO
        log_error(log_memoria, "no hay igual tamaño, busco menor"); //borrar
        segmento = list_get_minimum(segmentos, (void*)segmentoMenorTamanio); //ME DEVUELVE EL SEGMENTO DE MENOR TAMANIO DONDE ENTRE LO QUE QUIERO GUARDAR
    }

    
    return segmento;
}

t_segmento* segmentoMenorTamanio(t_segmento* segmento, t_segmento* otroSegmento){ //TODO aca estaban sin asterisco

    if(segmento->tamanio_segmento <= otroSegmento->tamanio_segmento){
        return segmento;
    }else
        return otroSegmento; 
}
t_segmento* segmentoMayorTamanio(t_segmento* segmento, t_segmento* otroSegmento){

    if(segmento->tamanio_segmento > otroSegmento->tamanio_segmento){
        return segmento;
    }else
        return otroSegmento; 
}
t_segmento* segmentoWorstFit(t_list* segmentos, int size){
    
    t_segmento* segmento;

    return segmento = list_get_maximum(segmentos, (void*)segmentoMayorTamanio);
}

void compactacion(){
    int base_aux=MEMORIA_PRINCIPAL + memoria_config.tam_segmento_0;
    int base_aux2=MEMORIA_PRINCIPAL + memoria_config.tam_segmento_0;
    log_trace(log_memoria, "entre en compactacion, base aux: %d",base_aux);
    //libero todo el bitmap
    liberarBitMap(memoria_config.tam_segmento_0-1, memoria_config.tam_memoria);
    //De todos los procesos

    for (int i = 0; i < list_size(tabla_de_procesos); i++)
    {
        log_error(log_memoria,"el tamanio de la lista  de procesoses: %d",list_size(tabla_de_procesos));
        t_proceso* unProceso = list_get(tabla_de_procesos, i);
        log_error(log_memoria,"el tamanio de la lista  de segmentos es: %d",list_size(unProceso->tabla_segmentos));
        //las tablas de segmentos
        for (int j = 1; j < list_size(unProceso->tabla_segmentos); j++)
        {
            t_segmento* unSegmento = list_get(unProceso->tabla_segmentos, j);
            unSegmento->direccion_base = base_aux;
            base_aux += unSegmento->tamanio_segmento;

        //- Resultado Compactación: Por cada segmento de cada proceso se deberá imprimir una línea con el siguiente formato:
        int base_log = unSegmento->direccion_base - base_aux2 + memoria_config.tam_segmento_0;
        log_info(log_memoria,"PID: %d - Segmento: %d - Base: %d - Tamaño %d, Baseliteral: %d", unProceso->id, unSegmento->id_segmento, base_log, unSegmento->tamanio_segmento, base_aux);
        }
    }

    ocuparBitMap(memoria_config.tam_segmento_0-1, (base_aux - (base_aux2)));// les resto 1 por el ultimo +1 de la iteracion del for
}

////
//// Hasta BITARRAYS no se usa nada
////
//CAMBIAR
t_list* buscarSegmentosOcupados(){
    t_list* segmentos = list_create();

    agregarProcesos(segmentos);

    return segmentos;
}
void agregarProcesos(t_list* segmentos){
    int tamanioListaProcesos = list_size(tabla_de_procesos);
    for (int i = 0; i < tamanioListaProcesos-1; i++)
    {
        t_proceso* unProceso = list_get(tabla_de_procesos, i);
        int tamanioListaSegmentosDelProceso = list_size(unProceso->tabla_segmentos);
        for (int j = 0; j < tamanioListaSegmentosDelProceso-1; j++)
        {
            t_segmento* unSegmento = list_get(unProceso->tabla_segmentos, j);
            list_add(segmentos, unSegmento);
        }
        
    }

}

t_list* copiarContenidoSeg(t_list* segmentosNoCompactados){

    t_list* segmentos = list_map(segmentosNoCompactados, (void*)copiarSegmentacion);
    return segmentos;

}

//CHEQUEAR

void* copiarSegmentacion(t_segmento* unSegmento){
	
	void* algo;
    //if(unSegmento->limite == 21){       //TRIPU
    //    algo = malloc(sizeof(t_tcb));
    //}else if(unSegmento->limite == 8){  //PATOTA
    //    algo = malloc(sizeof(t_pcb));
    //}else{                              //TAREAS
    //    algo = malloc(unSegmento->limite);
    //}

    algo = malloc(unSegmento->tamanio_segmento);
    //CHEQUEAR
	pthread_mutex_lock(&mutexMemoria);
	memcpy(algo,MEMORIA_PRINCIPAL + unSegmento->direccion_base, unSegmento->tamanio_segmento); //COPIA EN ALGO DESDE HASTA TAL LUGAR
	//CHEQUEAR
    pthread_mutex_unlock(&mutexMemoria);
    
    return algo;
}


//CHEQUEAR
//VER LA ACTUALIZACION DE LA COMPACTACION



void actualizarCompactacion(t_list* segmentosNoCompactados, t_list* segmentosCompactados){
    for(int i = 0; i<list_size(segmentosNoCompactados);i++){
        actualizarCadaSegmento(list_get(segmentosNoCompactados, i), list_get(segmentosCompactados,i));
    }
}
void actualizarCadaSegmento(t_segmento* segmentoViejo, t_segmento* segmentoNuevo){
    pthread_mutex_lock(&mutexMemoria);		
	log_info(log_memoria, "El segmento %d ahora tiene base %p",segmentoViejo->id_segmento, MEMORIA_PRINCIPAL + segmentoNuevo->direccion_base);
	pthread_mutex_unlock(&mutexMemoria);
    
    //CAMBIAR A FUNCIONES QUE SIRVAN O BORRAR
    //actualizarSegmento(segmentoViejo, segmentoNuevo); //si esta lo cambia, sino no hace nada
}

//void actualizarSegmento(t_segmento* viejo, t_segmento* nuevo){
//}


//void actualizarBase(t_segmento* segmento, t_segmento* otroSeg){
//
//    segmento->base= otroSeg->base;
//}




// BitArrays

//asiganrBits/Bytes
char *asignarMemoriaBits(int bits) // recibe bits asigna bytes
{
    char *aux;
    int bytes;
    bytes = bitsToBytes(bits);
    // printf("BYTES: %d\n", bytes);
    aux = malloc(bytes);
    memset(aux, 0, bytes);
    return aux;
}

char *asignarMemoriaBytes(int bytes){
    char *aux;
    aux = malloc(bytes);
    memset(aux, 0, bytes); // SETEA LOS BYTES EN 0
    return aux;
}

int bitsToBytes(int bits){
    int bytes;
    if (bits < 8)
        bytes = 1;
    else
    {
        double c = (double)bits;
        bytes = ceil(c / 8.0);
    }

    return bytes;
}

// bitMaps

void ocuparBitMap(int base, int size)
{
    log_warning(log_memoria,"entre en ocupar bitmap");
    
    pthread_mutex_lock(&mutexBitMapSegment);
    for (int i = 0; i < size; i++)
    {
        bitarray_set_bit(bitMapSegment, base + i); // REEMPLAZA LOS 0 POR 1, ASI SABEMOS QUE ESTA OCUPADO
    }
    pthread_mutex_unlock(&mutexBitMapSegment);
}

void liberarBitMap(int base, int size)
{
    log_warning(log_memoria, "entre en liberarBitmap");
    pthread_mutex_lock(&mutexBitMapSegment);
    for (int i = 0; i < size; i++)
    {
        bitarray_clean_bit(bitMapSegment, base + i); // REEMPLAZA LOS 1 POR 0, ASI SABEMOS QUE ESTA LIBRE
    }
    pthread_mutex_unlock(&mutexBitMapSegment);
}

int contarEspaciosLibresDesde(t_bitarray *bitmap, int i)
{ // CUENTA LOS 0 DEL BITMAP HASTA EL PRIMER 1 QUE ENCUENTRA

    int contador = 0;

    pthread_mutex_lock(&mutexBitMapSegment);

    while ((bitarray_test_bit(bitmap, i) == 0) && (i < (memoria_config.tam_memoria)))
    {

        // MIENTRAS EL BITMAP EN ESA POSICION SEA 0 Y NO NOS PASEMOS DE LOS LIMITES DE LA MEMORIA
        contador++;
        i++;
    }

    pthread_mutex_unlock(&mutexBitMapSegment);
    return contador;
}

int contarEspaciosOcupadosDesde(t_bitarray *unBitmap, int i)
{ // CUENTA LOS 1 DEL BITMAP HASTA EL PRIMER 0 QUE ENCUENTRA
    int contador = 0;

    while ((bitarray_test_bit(unBitmap, i) == 1) && (i < memoria_config.tam_memoria))
    {
        // MIENTRAS EL BITMAP EN ESA POSICION SEA 1 Y NO NOS PASEMOS DE LOS LIMITES DE LA MEMORIA
        contador++;
        i++;
    }

    return contador;
}


//listas => puede ir en el utils
void eliminarLista(t_list* lista){
	
	list_destroy_and_destroy_elements(lista, (void*)eliminarAlgo);
	

}
void eliminarAlgo(void* algo){
	
	free(algo);
	
}


void eliminarTablaDeProcesos(){
    list_destroy_and_destroy_elements(tabla_de_procesos, (void*)eliminarTablaDeSegmentos);
}
void eliminarTablaDeSegmentos(t_proceso* proceso){
    list_destroy_and_destroy_elements(proceso->tabla_segmentos, (void*)free);
    free(proceso);
}

int adaptar_base(t_segmento* unSegAux){
    int base = MEMORIA_PRINCIPAL;
    log_info(log_memoria, "la base es: %d", base);
    return unSegAux->direccion_base -= base; 
}
t_list* adaptar_TDP_salida(){
    t_list* tabla_de_procesos_aux = list_duplicate(tabla_de_procesos);
    for (int i = 0; i < list_size(tabla_de_procesos_aux); i++)
    {
       t_proceso* unProceso_aux = list_get(tabla_de_procesos_aux, i);

        list_map(unProceso_aux->tabla_segmentos, (void*)adaptar_base);
       
    }
    return tabla_de_procesos_aux;
}

*/