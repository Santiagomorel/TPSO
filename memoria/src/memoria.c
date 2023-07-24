#include "memoria.h"

t_config* config_memoria;
t_log *logger_memoria;
t_log *logger_memoria_extra;

int socket_servidor_memoria;

int PUERTO_ESCUCHA_MEMORIA;
int TAM_MEMORIA;
int TAM_SEGMENTO_0;
int CANT_SEGMENTOS;
int RETARDO_MEMORIA;
int RETARDO_COMPACTACION;
t_algo_asig ALGORITMO_ASIGNACION;

void* ESPACIO_USUARIO;
int ESPACIO_LIBRE_TOTAL;
t_list* LISTA_ESPACIOS_LIBRES;
t_list* LISTA_GLOBAL_SEGMENTOS;
pthread_mutex_t mutex_memoria;

int main(int argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "-test") == 0)
	{
		levantar_config_memoria();
		levantar_loggers_memoria();
		levantar_estructuras_administrativas();
		//run_tests();
		return EXIT_SUCCESS;
	}
	else
	{
		

		levantar_config_memoria();
		levantar_loggers_memoria();
		levantar_estructuras_administrativas();
		pthread_mutex_init(&mutex_memoria, NULL);



		//*********************
		// SERVIDOR
	socket_servidor_memoria = iniciar_servidor(PUERTO_ESCUCHA_MEMORIA, logger_memoria);
    log_trace(logger_memoria, "Servidor Memoria listo para recibir al cliente");

    pthread_t atiende_cliente_CPU, atiende_cliente_FILESYSTEM, atiende_cliente_KERNEL;

    log_trace(logger_memoria, "esperando cliente CPU");
    socket_cliente_memoria_CPU = esperar_cliente(socket_servidor_memoria, logger_memoria);
    pthread_create(&atiende_cliente_CPU, NULL, (void *)recibir_cpu, (void *)socket_cliente_memoria_CPU);
    pthread_detach(atiende_cliente_CPU);

    log_trace(logger_memoria, "esperando cliente fileSystem");
    socket_cliente_memoria_FILESYSTEM = esperar_cliente(socket_servidor_memoria, logger_memoria);
    pthread_create(&atiende_cliente_FILESYSTEM, NULL, (void *)recibir_fileSystem, (void *)socket_cliente_memoria_FILESYSTEM);
    pthread_detach(atiende_cliente_FILESYSTEM);

    log_trace(logger_memoria, "esperando cliente kernel");
    socket_cliente_memoria_KERNEL = esperar_cliente(socket_servidor_memoria, logger_memoria);
    pthread_create(&atiende_cliente_KERNEL, NULL, (void *)recibir_kernel, (void *)socket_cliente_memoria_KERNEL);
    pthread_detach(atiende_cliente_KERNEL);


    sem_wait(&finModulo);
    log_warning(logger_memoria, "FINALIZA EL MODULO DE MEMORIA");
    end_program();

    return 0;
	}
}


void devolver_tabla_inicial(int socket) {
    int size = sizeof(t_ent_ts) * CANT_SEGMENTOS + sizeof(int);
    void* buffer = malloc(size);

    memcpy(buffer, &CANT_SEGMENTOS, sizeof(int));

    void* tabla = crear_tabla_segmentos();

    memcpy(buffer + sizeof(int), tabla, sizeof(t_ent_ts) * CANT_SEGMENTOS);

    send(socket, buffer, size, NULL);

    free(buffer);
    free(tabla);

}

void devolver_resultado_creacion(int resultado, int socket, int base) {
    int tam_buffer = sizeof(int);
    if(resultado == MEMORIA_SEGMENTO_CREADO) {
        tam_buffer += sizeof(int);
    }
    void* buffer = malloc(tam_buffer);

    int despl = 0;

    memcpy(buffer, &resultado, sizeof(int));
    despl += sizeof(int);

    if(resultado == MEMORIA_SEGMENTO_CREADO) {
        memcpy(buffer + despl, &base, sizeof(int));
    }

    send(socket, buffer, tam_buffer, NULL);
    free(buffer);
}


void devolver_nuevas_bases(int cliente_socket) {
    int size = (sizeof(int) * 3) * list_size(LISTA_GLOBAL_SEGMENTOS) + sizeof(int);
    void* buffer = malloc(size);
    int desplazamiento = 0;
    
    memcpy(buffer, &size, sizeof(int));
    desplazamiento += sizeof(int);

    t_segmento_v2* segmento;
    // for each segmento in LISTA_GLOBAL_SEGMENTOS copy its pid, id and base
    for (int i = 0; i < list_size(LISTA_GLOBAL_SEGMENTOS); i++)
    {   
        segmento = list_get(LISTA_GLOBAL_SEGMENTOS, i);
        memcpy(buffer + desplazamiento, &segmento->pid, sizeof(int));
        desplazamiento += sizeof(int);
        memcpy(buffer + desplazamiento, &segmento->id, sizeof(int));
        desplazamiento += sizeof(int);
        memcpy(buffer + desplazamiento, &segmento->base, sizeof(int));
        desplazamiento += sizeof(int);
    }

    send(cliente_socket, buffer, size, NULL);
    free(buffer);
}
//----------------------------------------
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
            pthread_mutex_lock(&mutex_memoria);
            int pid = recibir_entero(SOCKET_CLIENTE_KERNEL, logger_memoria);
            devolver_tabla_inicial(SOCKET_CLIENTE_KERNEL);
            log_info(logger_memoria, "Creacion de Proceso PID: %d", pid);
            pthread_mutex_unlock(&mutex_memoria);

        break;
        
        case DELETE_PROCESS:
        int PID = recibir_entero(SOCKET_CLIENTE_KERNEL, logger_memoria);
        borrar_proceso(PID);
        break;
        case CREATE_SEGMENT:
            pthread_mutex_lock(&mutex_memoria);
            t_3_enteros* estructura = recibir_3_enteros(SOCKET_CLIENTE_KERNEL);

            int pid_create_segment = estructura->entero1;
            int id_seg= estructura->entero2;
            int tam_seg = estructura->entero3;
 
            int n_base;
            int resultado = crear_segmento_v2(tam_seg, &n_base);

            if (resultado == MEMORIA_SEGMENTO_CREADO)
            {
                t_segmento_v2* n_seg = malloc(sizeof(t_segmento_v2));
                n_seg->pid = pid_create_segment;
                n_seg->id = id_seg;
                n_seg->base = n_base;
                n_seg->limite = tam_seg;

                list_add_sorted(LISTA_GLOBAL_SEGMENTOS, n_seg, comparador_base_segmento);

                enviar_3enteros(SOCKET_CLIENTE_KERNEL, id_seg, n_base, tam_seg, OK);

                log_info(logger_memoria, "PID: %d - Crear Segmento: %d - Base: %d - TAMAÑO: %d", pid_create_segment, id_seg, n_base, tam_seg);

                
            }else if( resultado == NECESITO_COMPACTAR){
                enviar_CodOp(SOCKET_CLIENTE_KERNEL, NECESITO_COMPACTAR);
            }else if(resultado == OUT_OF_MEMORY){
                enviar_CodOp(SOCKET_CLIENTE_KERNEL, OUT_OF_MEMORY);
            }
            
            // print_lista_segmentos();
            // print_lista_esp(LISTA_ESPACIOS_LIBRES);

            devolver_resultado_creacion(resultado, SOCKET_CLIENTE_KERNEL, n_base);
            pthread_mutex_unlock(&mutex_memoria);

            break;

        case DELETE_SEGMENT:
            pthread_mutex_lock(&mutex_memoria);
            t_4_enteros* estructura_4_enteros = recibir_4_enteros(SOCKET_CLIENTE_KERNEL);


            int pid_free_segment = estructura_4_enteros->entero1;
            int free_seg_id= estructura_4_enteros->entero2;
            int tam= estructura_4_enteros->entero3;
            int base= estructura_4_enteros->entero4;

            borrar_segmento(base, tam);
            log_info(logger_memoria, "PID: %d - Eliminar Segmento: %d - Base: %d - TAMAÑO: %d", pid_free_segment, free_seg_id, base, tam);
            print_lista_esp(LISTA_ESPACIOS_LIBRES); 
            pthread_mutex_unlock(&mutex_memoria);
            break;

         case COMPACTAR:
            pthread_mutex_lock(&mutex_memoria);
            compactar();
            for(int i = 0; i < list_size(LISTA_GLOBAL_SEGMENTOS); i++)
            {
                t_segmento_v2* segmento = list_get(LISTA_GLOBAL_SEGMENTOS, i);
                log_info(logger_memoria, "PID: %d - Segmento: %d - Base: %d - Tamaño: %d", segmento->pid, segmento->id, segmento->base, segmento->limite);
            }
            devolver_nuevas_bases(SOCKET_CLIENTE_KERNEL); // VER ESTO
            print_lista_esp(LISTA_ESPACIOS_LIBRES);
            pthread_mutex_unlock(&mutex_memoria);
             break;

        case -1:
            codigoOP = codigoOperacion;
            break;
        // se desconecta kernel
        default:
            log_trace(logger_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(logger_memoria, "se desconecto kernel");
    sem_post(&finModulo);
}
//CPU
void recibir_cpu(int SOCKET_CLIENTE_CPU)
{

    enviar_mensaje("recibido cpu", SOCKET_CLIENTE_CPU);
    log_trace(logger_memoria, "recibido cpu");
    int codigoOP = 0;
    while (codigoOP != -1)
    {
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_CPU);
        sleep(RETARDO_MEMORIA);
        switch (codigoOperacion)
        {
        case MENSAJE:

            break;

        case MOV_IN: //(Registro, Direc_base ,size): Lee el valor de memoria correspondiente a la Dirección Lógica y lo almacena en el Registro.
            
            pthread_mutex_lock(&mutex_memoria);
            t_3_enteros* estructura_mov_in = recibir_3_enteros(SOCKET_CLIENTE_CPU);
            int pid_mov_in = estructura_mov_in->entero1;
            int dir_fisica_in = estructura_mov_in->entero2;
            int tam_a_leer = estructura_mov_in->entero3;

            char* valor_in = leer(dir_fisica_in, tam_a_leer);

            enviar_paquete_string(SOCKET_CLIENTE_CPU, valor_in, MOV_IN_OK, strlen(valor_in) + 1);

            free(valor_in);
            log_info(logger_memoria, "PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: CPU", pid_mov_in, dir_fisica_in, tam_a_leer);
            sleep(RETARDO_MEMORIA/1000);
            pthread_mutex_unlock(&mutex_memoria);

            break;
        case MOV_OUT: //(Dirección Fisica, Registro): Lee el valor del Registro y lo escribe en la dirección física de memoria obtenida a partir de la Dirección Lógica.
            
            pthread_mutex_lock(&mutex_memoria);
            t_string_3enteros* estructura_mov_out = recibir_string_3enteros(SOCKET_CLIENTE_CPU);
            int dir_fisica = estructura_mov_out->entero1;
            int tam_escrito = estructura_mov_out->entero3;
            char* valor = malloc(tam_escrito);
            valor = estructura_mov_out->string;
            int pid_mov_out = estructura_mov_out->entero2;
            
            // Para probar
            // char* cadena = imprimir_cadena(valor, tam_escrito);
            // printf("Valor recibido: %s\n", cadena);

            escribir(dir_fisica, valor, tam_escrito);
            log_info(logger_memoria, "PID: %d - Acción: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: CPU", pid_mov_out, dir_fisica, tam_escrito);
            sleep(RETARDO_MEMORIA/1000);
            free(valor);
            enviar_CodOp(SOCKET_CLIENTE_CPU, MOV_OUT_OK);
            //char* cosita = leer(dir_fisica, tam_escrito);
            pthread_mutex_unlock(&mutex_memoria);
            break;
        case -1:
        codigoOP = codigoOperacion;
        break;
        default:
            // log_trace(logger_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(logger_memoria, "se desconecto CPU");
}
//FILESYSTEM
void recibir_fileSystem(int SOCKET_CLIENTE_FILESYSTEM)
{

    enviar_mensaje("recibido fileSystem", SOCKET_CLIENTE_FILESYSTEM);
    int codigoOP = 0;
    while (codigoOP != -1)
    {
        int codigoOperacion = recibir_operacion(SOCKET_CLIENTE_FILESYSTEM);
        sleep(RETARDO_MEMORIA);
        log_warning(logger_memoria, "llegue a fs");
        switch (codigoOperacion)
        {
        case MENSAJE:

        break;
        case F_READ:
            pthread_mutex_lock(&mutex_memoria);
            t_string_3enteros* estructura_fread = recibir_string_3enteros;
            uint32_t tam_a_leer_archivo = estructura_fread->entero3;
            char* valor_leer_archivo = malloc(tam_a_leer_archivo);
            valor_leer_archivo = estructura_fread->string;
            uint32_t pid_leer_archivo = estructura_fread->entero1;
            uint32_t dir_fisica_leer_archivo = estructura_fread->entero2;


            escribir(dir_fisica_leer_archivo, valor_leer_archivo, tam_a_leer_archivo);
            free(valor_leer_archivo);
            log_info(logger_memoria, "PID: %d - Accion: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: FS", pid_leer_archivo, dir_fisica_leer_archivo, tam_a_leer_archivo);
            sleep(RETARDO_MEMORIA/1000);
            enviar_CodOp(SOCKET_CLIENTE_FILESYSTEM,OK);
            pthread_mutex_unlock(&mutex_memoria);
            break;
        
        case F_WRITE:
            pthread_mutex_lock(&mutex_memoria);
            t_3_enteros* estructura_fwrite = recibir_3_enteros(SOCKET_CLIENTE_FILESYSTEM);
            int pid_escribir_archivo = estructura_fwrite->entero1;
            int dir_fisica_escribir_archivo = estructura_fwrite->entero2;
            int tam_a_escribir_archivo = estructura_fwrite->entero3;

            char* valor_escribir_archivo = leer(dir_fisica_escribir_archivo, tam_a_escribir_archivo);

            log_info(logger_memoria, "PID: %d - Accion: LEER - Dirección física: %d - Tamaño: %d - Origen: FS", pid_escribir_archivo, dir_fisica_escribir_archivo, tam_a_escribir_archivo);
            sleep(RETARDO_MEMORIA/1000);
            enviar_paquete_string(SOCKET_CLIENTE_FILESYSTEM, valor_escribir_archivo,F_WRITE_OK, strlen(valor_escribir_archivo) + 1);

            free(valor_escribir_archivo);
            pthread_mutex_unlock(&mutex_memoria);
        break;

        case -1:
        codigoOP = codigoOperacion;
        break;

        default:
            log_trace(logger_memoria, "recibi el op_cod %d y entro DEFAULT", codigoOperacion);
            break;
        }
    }
    log_warning(logger_memoria, "se desconecto FILESYSTEM");
}

//---------------------------------------

char* t_algo_asig_desc[] = {"FIRST", "BEST", "WORST"};

void levantar_loggers_memoria() {
    logger_memoria = log_create("./runlogs/memoria.log", "MEMORIA", true, LOG_LEVEL_INFO);
    logger_memoria_extra = log_create("./runlogs/memoria_extra.log", "MEMORIA", true, LOG_LEVEL_INFO);
}

void levantar_config_memoria() {
    config_memoria = config_create("./config/memoria.config");
    PUERTO_ESCUCHA_MEMORIA = config_get_int_value(config_memoria, "PUERTO_ESCUCHA");
    TAM_MEMORIA = config_get_int_value(config_memoria, "TAM_MEMORIA");
    ESPACIO_LIBRE_TOTAL = TAM_MEMORIA;
    TAM_SEGMENTO_0 = config_get_int_value(config_memoria, "TAM_SEGMENTO_0");
    CANT_SEGMENTOS = config_get_int_value(config_memoria, "CANT_SEGMENTOS");
    RETARDO_MEMORIA = config_get_int_value(config_memoria, "RETARDO_MEMORIA");
    RETARDO_COMPACTACION = config_get_int_value(config_memoria, "RETARDO_COMPACTACION");
    char * algo_asig = config_get_string_value(config_memoria, "ALGORITMO_ASIGNACION");

    if (strcmp(algo_asig, "FIRST") == 0)
    {
        ALGORITMO_ASIGNACION = FIRST;
    } else if (strcmp(algo_asig, "BEST") == 0)
    {
        ALGORITMO_ASIGNACION = BEST;
    } else if (strcmp(algo_asig, "WORST") == 0)
    {
        ALGORITMO_ASIGNACION = WORST;
    } else {
        log_error(logger_memoria_extra, "ALGORITMO DE ASIGNACION DESCONOCIDO");
    }

    config_destroy(config_memoria);
}



void crear_segmento_0() {
    t_esp* espacio = list_get(LISTA_ESPACIOS_LIBRES, 0);
    espacio->base += TAM_SEGMENTO_0;
    espacio->limite -= TAM_SEGMENTO_0;
    ESPACIO_LIBRE_TOTAL -= TAM_SEGMENTO_0;
}

void levantar_estructuras_administrativas() {
    ESPACIO_USUARIO = malloc(TAM_MEMORIA);
    ESPACIO_LIBRE_TOTAL;

    LISTA_ESPACIOS_LIBRES = list_create();
    LISTA_GLOBAL_SEGMENTOS = list_create();

    t_esp* espacio_inicial = malloc(sizeof(t_esp));
    espacio_inicial->base = 0;
    espacio_inicial->limite = TAM_MEMORIA;

    list_add(LISTA_ESPACIOS_LIBRES, espacio_inicial);

    crear_segmento_0();
}

bool comparador_base(void* data1, void* data2) {
    t_esp* esp1 = (t_esp*)data1;
    t_esp* esp2 = (t_esp*)data2;

    return esp1->base < esp2->base;
}

bool comparador_base_segmento(void* data1, void* data2) {
    t_segmento_v2* seg1 = (t_segmento_v2*)data1;
    t_segmento_v2* seg2 = (t_segmento_v2*)data2;

    return seg1->base < seg2->base;
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
        t_segmento_v2* elemento = list_get(LISTA_GLOBAL_SEGMENTOS, i);
        printf("PID %u: ID=%u, BASE=%u, LIMITE=%u\n", elemento->pid, elemento->id, elemento->base, elemento->limite);
    }
}

void* crear_tabla_segmentos() {
    void* buffer = malloc(sizeof(t_ent_ts) * CANT_SEGMENTOS);
    int despl = 0;
    int cero = 0;
    int i = 0;
    uint8_t estado_inicial = 1;

    memcpy(buffer + despl,&i, sizeof(int)); // ID
    despl+=sizeof(int);
    i++;

    memcpy(buffer + despl,&cero, sizeof(int)); // BASE
    despl+=sizeof(int);

    memcpy(buffer + despl,&TAM_SEGMENTO_0, sizeof(int)); // LIMITE
    despl+=sizeof(int);

    memcpy(buffer + despl, &estado_inicial, sizeof(uint8_t));
    despl+=sizeof(uint8_t);

    estado_inicial = 0;

    for (; i < CANT_SEGMENTOS; i++)
    {
        memcpy(buffer + despl,&i, sizeof(int)); // ID
        despl+=sizeof(int);


        memcpy(buffer + despl,&cero, sizeof(int)); // BASE
        despl+=sizeof(int);

        memcpy(buffer + despl,&cero, sizeof(int)); // LIMITE
        despl+=sizeof(int);

        memcpy(buffer + despl, &estado_inicial, sizeof(uint8_t));
        despl+=sizeof(uint8_t);
    }
    
    return buffer;
}

int buscar_espacio_libre(int tam) {
    t_esp* esp;
    t_esp* esp_i;
    switch (ALGORITMO_ASIGNACION)
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

        log_info(logger_memoria_extra, "NO SE ENCONTRO UN ESPACIO LIBRE, SE NECESITA COMPACTAR");
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

        log_info(logger_memoria_extra, "NO SE ENCONTRO UN ESPACIO LIBRE, SE NECESITA COMPACTAR");
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

        log_info(logger_memoria_extra, "NO SE ENCONTRO UN ESPACIO LIBRE, SE NECESITA COMPACTAR");
        return -1;

        break;
    
    default:
        log_error(logger_memoria_extra,"ALGO BUSQUEDA ESPACIO DESCONOCIDO");
    }
    
    return -1;
}

int crear_segmento_v2(int tam, int* base_resultante) {
    //printf("%d, %d\n", ESPACIO_LIBRE_TOTAL, tam);
    if (ESPACIO_LIBRE_TOTAL < tam)
    {
        log_info(logger_memoria_extra, "NO HAY ESPACIO SUFICIENTE PARA CREAR ESE SEGMENTO");
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
    memcpy(base_resultante, &espacio->base, sizeof(int));

    espacio->base += tam;
    espacio->limite -= tam;
    ESPACIO_LIBRE_TOTAL -= tam;

    if (espacio->limite == 0)
    {
        list_remove(LISTA_ESPACIOS_LIBRES, i_espacio);
        free(espacio);
    }

    return MEMORIA_SEGMENTO_CREADO;
    
}

bool son_contiguos(t_esp* esp1, t_esp* esp2) {
    return esp1 ->base + esp1->limite == esp2 ->base;
}

int buscar_segmento_por_base(int base) {
    t_segmento_v2* segmento;
    for (int i = 0; i < list_size(LISTA_GLOBAL_SEGMENTOS); i++)
    {
        segmento = list_get(LISTA_GLOBAL_SEGMENTOS, i);
        if (segmento->base == base)
        {
            return i;
        }
    }
    return -1;
}


void borrar_segmento(int base, int limite) {
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

    t_segmento_v2* segmento = list_remove(LISTA_GLOBAL_SEGMENTOS,indice_segmento );
    free(segmento);
    
}

void escribir(uint32_t dir_fisca, void* data, uint32_t size) {
    memcpy(ESPACIO_USUARIO + dir_fisca, data, size);
}

char* leer(int dir_fisca , int size) {
    void* data = malloc(size);
    memcpy(data, ESPACIO_USUARIO + dir_fisca, size);
    return data;
}

void compactar() {
    for (int i = 0; i < list_size(LISTA_GLOBAL_SEGMENTOS); i++)
    {
        t_segmento_v2* segmento = list_get(LISTA_GLOBAL_SEGMENTOS, i);
        t_esp* primer_espacio_libre = list_get(LISTA_ESPACIOS_LIBRES, 0);
        if (segmento->base > primer_espacio_libre->base )
        {
            memcpy(ESPACIO_USUARIO + primer_espacio_libre->base, ESPACIO_USUARIO + segmento->base, segmento->limite);
            segmento->base = primer_espacio_libre->base;
            primer_espacio_libre->base += segmento->limite;
            
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

    sleep(RETARDO_COMPACTACION/1000);
}