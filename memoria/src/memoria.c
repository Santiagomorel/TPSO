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

    load_config();

    //log_trace(log_memoria, "cargo la configuracion de Memoria");

    levantar_estructuras_administrativas();

    //log_trace(log_memoria, "levanto las estructuras");
    
    pthread_mutex_init(&mutex_memoria, NULL);
    sem_init(&finModulo, 0, 0);


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
    
    enviar_CodOp(cliente_socket, OK_COMPACTACION);

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
        log_error(log_memoria,"ALGORITMO DE ASIGNACION DESCONOCIDO");
    }
    
    return -1;
}

op_code crear_segmento_memoria(uint32_t tam, uint32_t* base_resultante) {
    //printf("%d, %d\n", ESPACIO_LIBRE_TOTAL, tam);
    if (ESPACIO_LIBRE_TOTAL < tam)
    {
        //log_warning(log_memoria, "El espacio libre total de la memoria es: %d, mietras que el tamanio del parametro es: %d", ESPACIO_LIBRE_TOTAL, tam);
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

    //log_debug(log_memoria, "El espacio libre total despues de borar el segmento con tam, %d, es %d", limite, ESPACIO_LIBRE_TOTAL);
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




//manejo de conexiones

void recibir_kernel(int SOCKET_CLIENTE_KERNEL)
{

    enviar_mensaje("recibido kernel", SOCKET_CLIENTE_KERNEL);

    int codigoOP = 0;
    while (codigoOP != -1)
    {
        log_warning(log_memoria,"me quedo esperando en kernel"); //borrar despues de las pruebas finales
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_KERNEL);
        switch (codigoOperacion)
        {
        case INICIAR_ESTRUCTURAS:
            //log_trace(log_memoria, "recibi el op_cod %d INICIAR_ESTRUCTURAS", codigoOperacion);
            //- Creación de Proceso: “Creación de Proceso PID: <PID>”
            
            uint32_t pid = recibir_entero_u32(SOCKET_CLIENTE_KERNEL, log_memoria);
            log_trace(log_memoria, "Creacion de Proceso PID: %d", pid);
            devolver_tabla_inicial(SOCKET_CLIENTE_KERNEL);
            
            break;
        break;
        
        case CREATE_SEGMENT:
            
            t_3_enteros* create_data = recibir_3_u32(SOCKET_CLIENTE_KERNEL);
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
                log_trace(log_memoria, "PID: %d - Crear Segmento: %d - Base: %d - TAMAÑO: %d", pid_create_segment, id_seg, n_base, tam_seg);
            }

            devolver_resultado_creacion(resultado, SOCKET_CLIENTE_KERNEL, n_base);
            
            break;

        case DELETE_SEGMENT:
            // Debe recibir el id del segmento que desea eliminar
            
            t_4_enteros* delete_data = recibir_4_enteros(SOCKET_CLIENTE_KERNEL);
            uint32_t pid_free_segment = delete_data->entero1;
            uint32_t free_seg_id = delete_data->entero2;
            uint32_t base = delete_data->entero3;
            uint32_t tam = delete_data->entero4;

            borrar_segmento(base, tam);
            log_trace(log_memoria, "PID: %d - Eliminar Segmento: %d - Base: %d - TAMAÑO: %d", pid_free_segment, free_seg_id, base, tam);
            print_lista_esp(LISTA_ESPACIOS_LIBRES); //
            
            break;

         case COMPACTAR:
            
            compactar();
            for(int i = 0; i < list_size(LISTA_GLOBAL_SEGMENTOS); i++)
            {
                t_segmento_memoria* segmento = list_get(LISTA_GLOBAL_SEGMENTOS, i);
                log_trace(log_memoria, "PID: %d - Segmento: %d - Base: %d - Tamaño: %d", segmento->pid, segmento->id_segmento, segmento->direccion_base, segmento->tamanio_segmento);
            }
            devolver_nuevas_bases(SOCKET_CLIENTE_KERNEL);
            print_lista_esp(LISTA_ESPACIOS_LIBRES);
            
            break;

        case -1:
            codigoOP = codigoOperacion;
            break;
        // se desconecta kernel
        default:
            log_error(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(log_memoria, "se desconecto kernel");
    sem_post(&finModulo);
}

void recibir_cpu(int SOCKET_CLIENTE_CPU)
{

    enviar_mensaje("recibido cpu", SOCKET_CLIENTE_CPU);
    int codigoOP = 0;
    while (codigoOP != -1)
    {
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_CPU);
        switch (codigoOperacion)
        {
        case MENSAJE:
            log_info(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);
            break;

        case MOV_IN: 
            pthread_mutex_lock(&mutex_memoria);
            t_3_enteros* mov_in_data = recibir_3_enteros(SOCKET_CLIENTE_CPU);
            uint32_t pid_mov_in = mov_in_data->entero1;
            uint32_t dir_fisica_in = mov_in_data->entero2;
            uint32_t tam_a_leer = mov_in_data->entero3;
            char* valor_in = leer(dir_fisica_in, tam_a_leer);
            
            
            sleep(memoria_config.retardo_memoria/1000);
            enviar_paquete_string(SOCKET_CLIENTE_CPU, valor_in, MOV_IN_OK, tam_a_leer);
            free(valor_in);
            log_trace(log_memoria, "PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: CPU", pid_mov_in, dir_fisica_in, tam_a_leer);
            pthread_mutex_unlock(&mutex_memoria);
            break;


        case MOV_OUT: //(Dirección Fisica, Registro): Lee el valor del Registro y lo escribe en la dirección física de memoria obtenida a partir de la Dirección Lógica.
            pthread_mutex_lock(&mutex_memoria);
            t_string_3enteros* mov_out_data = recibir_string_3enteros(SOCKET_CLIENTE_CPU);
            uint32_t dir_fisica = mov_out_data->entero1;
            uint32_t pid_mov_out = mov_out_data->entero2;
            uint32_t tam_escrito = mov_out_data->entero3;
            char* escritura = mov_out_data->string;

            char* valor = malloc(tam_escrito);
            strcpy(valor,escritura);

            escribir(dir_fisica, valor, tam_escrito);
            log_trace(log_memoria, "PID: %d - Acción: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: CPU", pid_mov_out, dir_fisica, tam_escrito);
            free(valor);
            enviar_CodOp(SOCKET_CLIENTE_CPU, MOV_OUT_OK);
            sleep(memoria_config.retardo_memoria/1000);
            pthread_mutex_unlock(&mutex_memoria);
            break;

        case -1:
            codigoOP = codigoOperacion;
        break;
        
        default:
             log_error(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
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
        switch (codigoOperacion)
        {
        case MENSAJE:
            
            log_info(log_memoria, "recibi el op_cod %d MENSAJE , codigoOperacion", codigoOperacion);

            break;

        case F_READ:
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
            log_trace(log_memoria, "PID: %d - Accion: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: FS", pid_leer_archivo, dir_fisica_leer_archivo, tam_a_leer_archivo);
            sleep(memoria_config.retardo_memoria/1000);
            enviar_CodOp(SOCKET_CLIENTE_FILESYSTEM,F_READ_OK);
        pthread_mutex_unlock(&mutex_memoria);
            break;
        
        case F_WRITE:
            t_3_enteros* fwrite_data = recibir_3_enteros(SOCKET_CLIENTE_FILESYSTEM);
            uint32_t pid_escribir_archivo = fwrite_data->entero1;
            uint32_t dir_fisica_escribir_archivo = fwrite_data->entero2;
            uint32_t tam_a_escribir_archivo = fwrite_data -> entero3;

            char* valor_escribir_archivo = leer(dir_fisica_escribir_archivo, tam_a_escribir_archivo);
            log_trace(log_memoria, "PID: %d - Accion: LEER - Dirección física: %d - Tamaño: %d - Origen: FS", pid_escribir_archivo, dir_fisica_escribir_archivo, tam_a_escribir_archivo);
            sleep(memoria_config.retardo_memoria/1000);
            //enviar_paquete_string(SOCKET_CLIENTE_FILESYSTEM, valor_escribir_archivo, sizeof(valor_escribir_archivo), F_WRITE_OK);
            send(SOCKET_CLIENTE_FILESYSTEM, valor_escribir_archivo, tam_a_escribir_archivo, NULL);
            free(valor_escribir_archivo);
        pthread_mutex_unlock(&mutex_memoria);
            break;

        case -1:
        codigoOP = codigoOperacion;
        
        break;

        default:
            log_error(log_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(log_memoria, "se desconecto FILESYSTEM");
}

