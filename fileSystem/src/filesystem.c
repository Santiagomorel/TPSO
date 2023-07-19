#include "filesystem.h"

int main(int argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "-test") == 0)
	{
		levantar_loggers_filesystem();
		levantar_config_filesystem();
		levantar_superbloque();
		bitmap = levantar_bitmap();
		blocks_buffer = levantar_bloques();
		//run_tests();
		
	}
	else
	{
		// Rutinas de preparación
		levantar_loggers_filesystem();
		levantar_config_filesystem();
		levantar_superbloque();

    log_trace(logger_filesystem, "Estoy antes de establecer conexion");

		establecer_conexion(logger_filesystem);

    log_trace(logger_filesystem, "Estoy despues de establecer conexion");
		//*********************
		// PREPARACIÓN DE BLOQUES - TODO: Hacer logs
		bitmap = levantar_bitmap();
    log_trace(logger_filesystem, "Estoy despues de levantar bitmap");
		blocks_buffer = levantar_bloques();
    log_trace(logger_filesystem, "Estoy despues de levantar bloques");
		// Prueba de archivo de bloques
		// truncar_archivo("elpicante", 0);

		// for (int i = 0; i < BLOCK_COUNT; i++)
		// {
  
		// 	printf("Pos. %d: %d\n", i + 1, bitarray_test_bit(bitmap, i));
		// }

		// for (int i = 0; i < 5; i++)
		// {
		// 	uint32_t puntero;
		// 	memcpy(&puntero, blocks_buffer + BLOCK_SIZE + sizeof(uint32_t) * i, sizeof(uint32_t));
		// 	printf("Puntero %d: %d\n", i + 1, puntero);
		// }

		//*********************
		// SERVIDOR
		/*
    socket_servidor_filesystem = iniciar_servidor(logger_filesystem, PUERTO_ESCUCHA_FILESYSTEM);
		if (socket_servidor_filesystem == -1)
		{
			log_trace(logger_filesystem, "No se pudo iniciar el servidor en Filesystem...");
			return EXIT_FAILURE;
		}
		log_info(logger_filesystem, "Filesystem escuchando conexiones...");
		while (server_escuchar(logger_filesystem, socket_servidor_filesystem, (void *)procesar_conexion));
*/
    pthread_t threadDispatch;

    pthread_create(&threadDispatch, NULL, (void *) procesar_conexion, NULL);
    pthread_join(threadDispatch, NULL);
 
  log_trace(logger_filesystem, "Estoy despues de levantar como servidor");

		bitarray_destroy(bitmap);
		free(blocks_buffer);
		config_destroy(CONFIG_FILESYSTEM);
		return EXIT_SUCCESS;
	}
}

void levantar_loggers_filesystem()
{
  logger_filesystem = log_create("./runlogs/filesystem.log", "FILESYSTEM", true, LOG_LEVEL_INFO);
}

void levantar_config_filesystem()
{
 
  log_trace(logger_filesystem, "intentamo levantar config");
  CONFIG_FILESYSTEM = config_create("./config/filesystem.config");
  log_trace(logger_filesystem, "se hace el create");

  IP_MEMORIA = config_get_string_value(CONFIG_FILESYSTEM, "IP_MEMORIA");
  PUERTO_MEMORIA = config_get_string_value(CONFIG_FILESYSTEM, "PUERTO_MEMORIA");
  PUERTO_ESCUCHA_FILESYSTEM = config_get_string_value(CONFIG_FILESYSTEM, "PUERTO_ESCUCHA");
  PATH_SUPERBLOQUE = config_get_string_value(CONFIG_FILESYSTEM, "PATH_SUPERBLOQUE");
  PATH_BITMAP = config_get_string_value(CONFIG_FILESYSTEM, "PATH_BITMAP");
  PATH_BLOQUES = config_get_string_value(CONFIG_FILESYSTEM, "PATH_BLOQUES");
  PATH_FCB = config_get_string_value(CONFIG_FILESYSTEM, "PATH_FCB");
  RETARDO_ACCESO_BLOQUE = config_get_int_value(CONFIG_FILESYSTEM, "RETARDO_ACCESO_BLOQUE");

  log_trace(logger_filesystem, "termine de crear config");
}

void levantar_superbloque()
{

  CONFIG_SUPERBLOQUE = config_create(PATH_SUPERBLOQUE);
  log_trace(logger_filesystem, "Cree config superbloque con path: %s", PATH_SUPERBLOQUE);
  BLOCK_SIZE = config_get_int_value(CONFIG_SUPERBLOQUE, "BLOCK_SIZE");
  BLOCK_COUNT = config_get_int_value(CONFIG_SUPERBLOQUE, "BLOCK_COUNT");
  config_destroy(CONFIG_SUPERBLOQUE);
  log_trace(logger_filesystem, "Termine de crear superbloque");
  log_trace(logger_filesystem, "Borro config");
  
}

t_bitarray *levantar_bitmap()
{
  int bitmap_size = BLOCK_COUNT / 8;
  void *puntero_a_bits = malloc(bitmap_size);

  t_bitarray *bitarray = bitarray_create_with_mode(puntero_a_bits, bitmap_size, LSB_FIRST);
  for (int i = 0; i < BLOCK_COUNT; i++)
  {
    bitarray_clean_bit(bitarray, i);
  }

  FILE *bitmap_file = fopen(PATH_BITMAP, "rb");
  if (bitmap_file == NULL)
  {
    bitmap_file = fopen(PATH_BITMAP, "wb");
    fwrite(bitarray->bitarray, 1, bitmap_size, bitmap_file);
  }

  fread(bitarray->bitarray, 1, bitmap_size, bitmap_file);

  // for (int i = 0; i < BLOCK_COUNT; i++)
  // {
  //   printf("Pos. %d: %d\n", i + 1, bitarray_test_bit(bitarray, i));
  // }

  fclose(bitmap_file);
  // free(puntero_a_bits); TODO: Liberar esto al final del programa
  return bitarray;
  
}

void ocupar_bloque(int numero_bloque)
{
  int bitmap_size = BLOCK_COUNT / 8;

  bitarray_set_bit(bitmap, numero_bloque);

  FILE *bitmap_file = fopen(PATH_BITMAP, "r+b");
  fwrite(bitmap->bitarray, 1, bitmap_size, bitmap_file);

  log_info(logger_filesystem, "Acceso a Bitmap - Bloque: %d - Estado: %d", numero_bloque, 1);

  fclose(bitmap_file);
}

void desocupar_bloque(int numero_bloque)
{
  int bitmap_size = BLOCK_COUNT / 8;

  bitarray_clean_bit(bitmap, numero_bloque);

  FILE *bitmap_file = fopen(PATH_BITMAP, "r+b");
  fwrite(bitmap->bitarray, 1, bitmap_size, bitmap_file);

  log_info(logger_filesystem, "Acceso a Bitmap - Bloque: %d - Estado: %d", numero_bloque, 0);

  fclose(bitmap_file);

  char *bloque_limpio = calloc(1, BLOCK_SIZE);
  modificar_bloque(numero_bloque * BLOCK_SIZE, bloque_limpio);
}

char *levantar_bloques()
{
  char *blocks_buffer = calloc(BLOCK_COUNT, BLOCK_SIZE); // Crea un buffer con todo inicializado en ceros

  FILE *blocks_file = fopen(PATH_BLOQUES, "r");
  if (blocks_file == NULL)
  {
    blocks_file = fopen(PATH_BLOQUES, "w");
    fwrite(blocks_buffer, BLOCK_SIZE, BLOCK_COUNT, blocks_file);
  }

  fread(blocks_buffer, BLOCK_SIZE, BLOCK_COUNT, blocks_file);

  fclose(blocks_file);
  return blocks_buffer;
}

char *leer_bloque(uint32_t puntero_a_bloque)
{
  uint32_t offset = puntero_a_bloque;
  char *bloque_leido = malloc(BLOCK_SIZE);
  memcpy(bloque_leido, blocks_buffer + offset, BLOCK_SIZE);
  
  return bloque_leido;
}

void modificar_bloque(uint32_t puntero_a_bloque, char *bloque_nuevo)
{
  size_t offset = puntero_a_bloque;
  memcpy(blocks_buffer + offset, bloque_nuevo, BLOCK_SIZE);

  FILE *blocks_file = fopen(PATH_BLOQUES, "r+");
  fseek(blocks_file, offset, SEEK_SET);
  fwrite(blocks_buffer + offset, BLOCK_SIZE, 1, blocks_file);

  free(bloque_nuevo);
  fclose(blocks_file);
}

t_fcb *levantar_fcb(char *f_name)
{
  char* path; // 46 viene de los caracteres de: ./fs/fcb/f_name.config
  strcpy(path, PATH_FCB);
  strcat(path, "/");
  strcat(path, f_name);
  strcat(path, ".config");

  t_config *FCB = config_create(path);
  t_fcb *fcb = malloc(sizeof(t_fcb));
  strncpy(fcb->f_name, f_name, 29); // Si la cadena de origen tiene menos de 29 caracteres, los faltantes se llenan con caracteres nulos
  fcb->f_name = '\0';           // Agrega el carácter nulo al final
  fcb->f_size = config_get_int_value(FCB, "TAMANIO_ARCHIVO");
  fcb->f_dp = config_get_int_value(FCB, "PUNTERO_DIRECTO");
  fcb->f_ip = config_get_int_value(FCB, "PUNTERO_INDIRECTO");
  config_destroy(FCB);

  return fcb;
}

int calcular_bloques_por_size(uint32_t size)
{
  int resultado = size / BLOCK_SIZE;
  if (size % BLOCK_SIZE != 0)
  {
    resultado++;
  }
  return resultado;
}

int encontrar_bloque_libre()
{
  for (int i = 0; i < BLOCK_COUNT; i++)
  {
    if (bitarray_test_bit(bitmap, i) == 0)
    {
      log_info(logger_filesystem, "Acceso a Bitmap - Bloque: %d - Estado: %d", i, 0);
      return i;
    }
  }

  // No se encontraron bloques libres
  return -1;
}

void asignar_bloque_directo(t_fcb *fcb)
{
  int bloque_libre = encontrar_bloque_libre(bitmap);
  if (bloque_libre != -1)
  {
    ocupar_bloque(bloque_libre);
    fcb->f_dp = BLOCK_SIZE * bloque_libre;
    printf("Puntero directo: %d\n", fcb->f_dp);
  }
}

void asignar_bloque_indirecto(t_fcb *fcb)
{
  int bloque_libre = encontrar_bloque_libre(bitmap);
  if (bloque_libre != -1)
  {
    ocupar_bloque(bloque_libre);
    fcb->f_ip = BLOCK_SIZE * bloque_libre;
    printf("Puntero indirecto: %d\n", fcb->f_ip);
  }
}

void asignar_bloque_al_bloque_indirecto(t_fcb *fcb, int bloques_ya_asignados)
{
  int bloque_libre = encontrar_bloque_libre(bitmap);
  if (bloque_libre != -1 && (sizeof(uint32_t) * bloques_ya_asignados + 1) < BLOCK_SIZE)
  {
    ocupar_bloque(bloque_libre);

    uint32_t puntero_nb = BLOCK_SIZE * bloque_libre;
    size_t offset = sizeof(uint32_t) * bloques_ya_asignados;
    char *bloque_a_modificar = leer_bloque(fcb->f_ip);
    memcpy(bloque_a_modificar + offset, &puntero_nb, sizeof(uint32_t));
    modificar_bloque(fcb->f_ip, bloque_a_modificar);

    printf("Puntero a bloque nuevo: %d\n", puntero_nb);
  }
}

void remover_puntero_de_bloque_indirecto(t_fcb *fcb, int bloques_utiles)
{
  char *bloque_a_rellenar = calloc(1, BLOCK_SIZE);
  memcpy(bloque_a_rellenar, blocks_buffer + fcb->f_ip, sizeof(uint32_t) * bloques_utiles);
  modificar_bloque(fcb->f_ip, bloque_a_rellenar);
}

void establecer_conexion(t_log* logger){

	log_trace(logger, "Inicio como cliente");

	log_trace(logger_filesystem,"Lei la IP %s , el Puerto Memoria %s ", IP_MEMORIA, PUERTO_MEMORIA);

	/*----------------------------------------------------------------------------------------------------------------*/
  log_trace(logger_filesystem, "Estoy adentro de establecer conexion");
  
	
	if((socket_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA)) == -1){
    log_trace(logger_filesystem, "Entre al if de establecer conexion");
		log_trace(logger, "Error al conectar con Memoria. El servidor no esta activo");
        free(socket_memoria);
		exit(-1);
	}else{
    log_trace(logger_filesystem, "Entre al else de establecer conexion");
		//handshake_cliente(conexion_cpu);
		enviar_mensaje(IP_MEMORIA, socket_memoria);
	}

    recibir_operacion(socket_memoria);
    recibir_mensaje(socket_memoria, logger_filesystem);


}

/******************COMUNICACION******************/

void procesar_conexion()
{
    log_trace(logger_filesystem, "Estoy antes de iniciar servidor");

    socket_servidor_filesystem = iniciar_servidor(PUERTO_ESCUCHA_FILESYSTEM, logger_filesystem);

    log_trace(logger_filesystem, "inicio servidor");
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
            log_trace(logger_filesystem, "recibi el nombre de archivo");

            archivo_ok = abrir_archivo(nombre_archivo);
            log_trace(logger_filesystem, "abrimos el archivo");

            if(archivo_ok){
                enviar_CodOp(cliente_socket, NO_EXISTE_ARCHIVO);
                log_trace(logger_filesystem, "archivo no encontrado");
            }else{
                enviar_CodOp(cliente_socket, EXISTE_ARCHIVO);
                log_trace(logger_filesystem, "archivo encontrado");
            }


            log_trace(logger, "Abrir archivo: %s", nombre_archivo);
            break;
        case F_CREATE:
            char* nombre_archivo2 = recibir_string(cliente_socket, logger);

            crear_archivo(nombre_archivo);

            log_trace(logger, "Crear archivo: %s", nombre_archivo);
            break;
        case F_TRUNCATE:
            t_string_entero* estructura = recibir_string_entero(cliente_socket);
            log_trace(logger_filesystem, "archivo recibido");
            truncar_archivo(estructura->string, (uint32_t)estructura->entero1);
            log_trace(logger_filesystem, "archivo truncado");
            break;
        case F_READ:
            t_string_4enteros* estructura_string_4enteros_l = recibir_string_4enteros(cliente_socket);
            log_trace(logger_filesystem, "archivo recibido con sus parametros");
            char* f_name = estructura_string_4enteros_l->string;
            uint32_t pid = estructura_string_4enteros_l->entero4;
            uint32_t offset = estructura_string_4enteros_l->entero1;
            uint32_t dir_fisica = estructura_string_4enteros_l->entero3;
            uint32_t cant = estructura_string_4enteros_l->entero2;


            log_trace(logger, "Leer archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d" ,f_name, offset, dir_fisica, cant);
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
            log_trace(logger_filesystem, "enviamos el empaquetado de datos");
            /*
            if(send(socket_memoria, buffer_leido, sizeof(uint32_t)*3 + cant + sizeof(int), NULL) == -1 ) {
            printf("Hubo un problema\n");
            }*/
            
            //recv(socket_memoria, &archivo_ok, sizeof(uint32_t), NULL);

            int cod_op = recibir_operacion(socket_memoria);
            log_trace(logger_filesystem, "recibimos operacion de memoria");
            enviar_CodOp(cliente_socket, cod_op); 
            log_trace(logger_filesystem, "enviamos codigo ");
            //free(buffer_leido);
            free(stream_leido);
            break;
        case F_WRITE:
            t_string_4enteros* estructura_string_4enteros_e = recibir_string_4enteros(cliente_socket);
            log_trace(logger_filesystem, "archivo recibido con sus parametros");
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
            log_trace(logger_filesystem, "enviamos el empaquetado de datos");
            //send(socket_memoria, buffer_memoria, sizeof(int) + sizeof(uint32_t)*3, NULL);
            //free(buffer_memoria);

            void* buffer_escritura = malloc(cant2);
            recv(socket_memoria, buffer_escritura, cant2, NULL);
            log_trace(logger_filesystem, "recibimos operacion de memoria");
            eferrait(f_name2, offset2, cant2, buffer_escritura);
            log_trace(logger_filesystem, "aplicamos el F_WRITE");
            free(buffer_escritura);
            
            enviar_CodOp(cliente_socket, OK);
            log_trace(logger_filesystem, "enviamos codigo de operacion a kernel");
            break;
        default:
            log_trace(logger, "Algo anduvo mal en el server de Filesystem");
            log_info(logger, "Cop: %d", cop);
            return;
        }
    }

    log_warning(logger, "El cliente se desconectó del server");
    return;
}

/******************CORE******************/

uint32_t abrir_archivo(char* f_name)
{
  char* path; // 46 viene de los caracteres de: ./fs/fcb/f_name.config
  strcpy(path, PATH_FCB);
  strcat(path, "/");
  strcat(path, f_name);
  strcat(path, ".config");

  FILE *archivo_fcb = fopen(path, "r");
  if (archivo_fcb == NULL)
  {
    //printf("El archivo no existe\n");
    return 1;
  }
  //printf("Archivo abierto\n");
  fclose(archivo_fcb);
  return 0;
}

uint32_t crear_archivo(char* f_name)
{
  t_fcb *new_fcb = malloc(sizeof(t_fcb));
  strncpy(new_fcb->f_name, f_name, 29); // Si la cadena de origen tiene menos de 29 caracteres, los faltantes se llenan con caracteres nulos
  new_fcb->f_name = '\0';           // Agrega el carácter nulo al final
  new_fcb->f_size = 0;
  new_fcb->f_dp = 0;
  new_fcb->f_ip = 0;

  char* path; // 46 viene de los caracteres de: ./fs/fcb/f_name.config
  strcpy(path, PATH_FCB);
  strcat(path, "/");
  strcat(path, f_name);
  strcat(path, ".config");

  FILE *archivo_fcb = fopen(path, "w");
  fprintf(archivo_fcb, "NOMBRE_ARCHIVO=%s\n", new_fcb->f_name);
  fprintf(archivo_fcb, "TAMANIO_ARCHIVO=%u\n", new_fcb->f_size);
  fprintf(archivo_fcb, "PUNTERO_DIRECTO=%u\n", new_fcb->f_dp);
  fprintf(archivo_fcb, "PUNTERO_INDIRECTO=%u\n", new_fcb->f_ip);

  // printf("Archivo creado\n");
  fclose(archivo_fcb);
  free(new_fcb);
  return 0;
}

void truncar_archivo(char *f_name, uint32_t new_size)
{
  log_info(logger_filesystem, "Truncar archivo: %s - Tamaño: %d", f_name, new_size);
  t_fcb *fcb = levantar_fcb(f_name);

  uint32_t prev_size = fcb->f_size;
  fcb->f_size = new_size;

  if (prev_size < new_size)
  {
    int bloques_actuales = calcular_bloques_por_size(prev_size);
    int bloques_necesarios = calcular_bloques_por_size(new_size);
    int bloques_adicionales = bloques_necesarios - bloques_actuales;

    if (bloques_adicionales > 0)
    {
      if (bloques_actuales == 0)
      {
        asignar_bloque_directo(fcb);
        if (bloques_adicionales > 1)
        {
          asignar_bloque_indirecto(fcb);
          log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d (indirecto) - Bloque File System %d", f_name, 1, fcb->f_ip / BLOCK_SIZE);
          sleep(RETARDO_ACCESO_BLOQUE / 1000);
          for (int i = 0; i < bloques_adicionales - 1; i++)
          {
            asignar_bloque_al_bloque_indirecto(fcb, i);
          }
        }
        

      }
      else
      {
        if (bloques_actuales == 1)
        {
          asignar_bloque_indirecto(fcb);
        }
        
        log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d (indirecto) - Bloque File System %d", f_name, 1, fcb->f_ip / BLOCK_SIZE);
        sleep(RETARDO_ACCESO_BLOQUE / 1000);
        for (int i = 0; i < bloques_adicionales; i++)
        {
          asignar_bloque_al_bloque_indirecto(fcb, i + bloques_actuales - 1);
        }
      }
    }
  }
  else if (prev_size > new_size)
  {
    int bloques_actuales = calcular_bloques_por_size(prev_size);
    int bloques_necesarios = calcular_bloques_por_size(new_size);
    int bloques_sobrantes = bloques_actuales - bloques_necesarios;

    if (bloques_sobrantes > 0)
    {
      if (bloques_necesarios == 0)
      {
        for (int i = (bloques_sobrantes - 2); i >= 0; i--)
        {
          uint32_t puntero;
          memcpy(&puntero, blocks_buffer + fcb->f_ip + sizeof(uint32_t) * i, sizeof(uint32_t));
          desocupar_bloque(puntero / BLOCK_SIZE);
          remover_puntero_de_bloque_indirecto(fcb, i);
        }
        log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d (indirecto) - Bloque File System %d", f_name, 1, fcb->f_ip / BLOCK_SIZE);
        sleep(RETARDO_ACCESO_BLOQUE / 1000);

        desocupar_bloque(fcb->f_ip / BLOCK_SIZE);
        fcb->f_ip = 0;
        desocupar_bloque(fcb->f_dp / BLOCK_SIZE);
        fcb->f_dp = 0;
      }
      else
      {
        log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d (indirecto) - Bloque File System %d", f_name, 1, fcb->f_ip / BLOCK_SIZE);
        sleep(RETARDO_ACCESO_BLOQUE / 1000);
        for (int i = (bloques_actuales - 2); i > (bloques_necesarios - 2); i--)
        {
          uint32_t puntero;
          memcpy(&puntero, blocks_buffer + fcb->f_ip + sizeof(uint32_t) * i, sizeof(uint32_t));
          desocupar_bloque(puntero / BLOCK_SIZE);
          remover_puntero_de_bloque_indirecto(fcb, i);
        }
      }
    }
  }

  char* path; // 46 viene de los caracteres de: ./fs/fcb/f_name.config
  strcpy(path, PATH_FCB);
  strcat(path, "/");
  strcat(path, f_name);
  strcat(path, ".config");

  FILE *archivo_fcb = fopen(path, "w");
  fprintf(archivo_fcb, "NOMBRE_ARCHIVO=%s\n", fcb->f_name);
  fprintf(archivo_fcb, "TAMANIO_ARCHIVO=%u\n", fcb->f_size);
  fprintf(archivo_fcb, "PUNTERO_DIRECTO=%u\n", fcb->f_dp);
  fprintf(archivo_fcb, "PUNTERO_INDIRECTO=%u\n", fcb->f_ip);

  free(fcb);
  fclose(archivo_fcb);
}

void eferrait(char *f_name, uint32_t offset, uint32_t cantidad, char *data)
{
  t_fcb *fcb = levantar_fcb(f_name);
  int desplazamiento = 0;
  int bloque_inicial = offset / BLOCK_SIZE;
  int bloque_final = (offset + cantidad) / BLOCK_SIZE;

  if (bloque_inicial == 0 && bloque_final == 0)
  {
    memcpy(blocks_buffer + fcb->f_dp + offset, data, cantidad);
    log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System %d", f_name, 0, fcb->f_dp / BLOCK_SIZE);
    sleep(RETARDO_ACCESO_BLOQUE / 1000);
  }
  else if (bloque_inicial == bloque_final)
  {
    uint32_t puntero_a_escribir;
    memcpy(&puntero_a_escribir, blocks_buffer + fcb->f_ip + sizeof(uint32_t) * (bloque_inicial - 1), sizeof(uint32_t));
    log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d (indirecto) - Bloque File System %d", f_name, 1, fcb->f_ip / BLOCK_SIZE);
    sleep(RETARDO_ACCESO_BLOQUE / 1000);

    memcpy(blocks_buffer + puntero_a_escribir + (offset - BLOCK_SIZE * bloque_inicial), data, cantidad);
    log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System %d", f_name, bloque_inicial + 1, puntero_a_escribir / BLOCK_SIZE);
    sleep(RETARDO_ACCESO_BLOQUE / 1000);
  }
  else
  {
    int bloques_a_escribir = (bloque_final - bloque_inicial) + 1;
    int cantidad_restante = cantidad;
    int desplazamiento = 0;
    if (bloque_inicial == 0)
    {
      memcpy(blocks_buffer + fcb->f_dp + offset, data, BLOCK_SIZE - offset);
      desplazamiento += BLOCK_SIZE - offset;
      cantidad_restante -= desplazamiento;
      log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System %d", f_name, 0, fcb->f_dp / BLOCK_SIZE);
      sleep(RETARDO_ACCESO_BLOQUE / 1000);
    }
    else
    {
      uint32_t puntero_primer_bloque_a_escribir;
      memcpy(&puntero_primer_bloque_a_escribir, blocks_buffer + fcb->f_ip + sizeof(uint32_t) * (bloque_inicial - 1), sizeof(uint32_t));
      log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d (indirecto) - Bloque File System %d", f_name, 1, fcb->f_ip / BLOCK_SIZE);
      sleep(RETARDO_ACCESO_BLOQUE / 1000);

      memcpy(blocks_buffer + puntero_primer_bloque_a_escribir + (offset - BLOCK_SIZE * bloque_inicial), data + desplazamiento, BLOCK_SIZE * (bloque_inicial + 1) - offset);
      log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System %d", f_name, bloque_inicial + 1, puntero_primer_bloque_a_escribir / BLOCK_SIZE);
      sleep(RETARDO_ACCESO_BLOQUE / 1000);

      desplazamiento = +BLOCK_SIZE * (bloque_inicial + 1) - offset;
      cantidad_restante -= BLOCK_SIZE * (bloque_inicial + 1) - offset;
    }
    bloques_a_escribir--;

    for (int i = bloque_inicial + 1; i <= bloque_final; i++)
    {
      uint32_t puntero;
      memcpy(&puntero, blocks_buffer + fcb->f_ip + sizeof(uint32_t) * (i-1), sizeof(uint32_t));
      int cant_a_escribir = cantidad_restante > BLOCK_SIZE ? BLOCK_SIZE : cantidad_restante;
      memcpy(blocks_buffer + puntero, data + desplazamiento, cant_a_escribir);
      desplazamiento += cant_a_escribir;
      cantidad_restante -= cant_a_escribir;
      log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System %d", f_name, i + 1, puntero / BLOCK_SIZE);
      sleep(RETARDO_ACCESO_BLOQUE / 1000);
    }
  }

  // Actualizar block_file
  FILE *blocks_file = fopen(PATH_BLOQUES, "w");
  fwrite(blocks_buffer, BLOCK_SIZE, BLOCK_COUNT, blocks_file);
  fclose(blocks_file);
  free(fcb);
}

void *eferrid(char *f_name, uint32_t offset, uint32_t cantidad)
{
  void *data_final = malloc(cantidad);
  t_fcb *fcb = levantar_fcb(f_name);
  int desplazamiento = 0;
  int bloque_inicial = offset / BLOCK_SIZE;
  int bloque_final = (offset + cantidad) / BLOCK_SIZE;

  if (bloque_inicial == 0 && bloque_final == 0)
  {
    memcpy(data_final, blocks_buffer + fcb->f_dp + offset, cantidad);
    log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System %d", f_name, 0, fcb->f_dp / BLOCK_SIZE);
    sleep(RETARDO_ACCESO_BLOQUE / 1000);
  }
  else if (bloque_inicial == bloque_final)
  {
    uint32_t puntero_a_leer;
    memcpy(&puntero_a_leer, blocks_buffer + fcb->f_ip + sizeof(uint32_t) * (bloque_inicial - 1), sizeof(uint32_t));
    log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d (indirecto) - Bloque File System %d", f_name, 1, fcb->f_ip / BLOCK_SIZE);
    sleep(RETARDO_ACCESO_BLOQUE / 1000);

    memcpy(data_final, blocks_buffer + puntero_a_leer + (offset - BLOCK_SIZE * bloque_inicial), cantidad);
    log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System %d", f_name, bloque_inicial + 1, puntero_a_leer / BLOCK_SIZE);
    sleep(RETARDO_ACCESO_BLOQUE / 1000);
  }
  else
  {
    int bloques_a_leer = (bloque_final - bloque_inicial) + 1;
    int cantidad_restante = cantidad;
    int desplazamiento = 0;

    if (bloque_inicial == 0)
    {
      memcpy(data_final, blocks_buffer + fcb->f_dp + offset, BLOCK_SIZE - offset);
      desplazamiento += BLOCK_SIZE - offset;
      cantidad_restante -= desplazamiento;
      log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System %d", f_name, 0, fcb->f_dp / BLOCK_SIZE);
      sleep(RETARDO_ACCESO_BLOQUE / 1000);
    }
    else
    {
      uint32_t puntero_primer_bloque_a_leer;
      memcpy(&puntero_primer_bloque_a_leer, blocks_buffer + fcb->f_ip + sizeof(uint32_t) * (bloque_inicial - 1), sizeof(uint32_t));
      log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d (indirecto) - Bloque File System %d", f_name, 1, fcb->f_ip / BLOCK_SIZE);
      sleep(RETARDO_ACCESO_BLOQUE / 1000);

      memcpy(data_final + desplazamiento, blocks_buffer + puntero_primer_bloque_a_leer + (offset - BLOCK_SIZE * bloque_inicial), BLOCK_SIZE * (bloque_inicial + 1) - offset);
      desplazamiento += BLOCK_SIZE * (bloque_inicial + 1) - offset;
      cantidad_restante -= BLOCK_SIZE * (bloque_inicial + 1) - offset;
      log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System %d", f_name, bloque_inicial + 1, puntero_primer_bloque_a_leer / BLOCK_SIZE);
      sleep(RETARDO_ACCESO_BLOQUE / 1000);
    }
    bloques_a_leer--;

    for (int i = bloque_inicial + 1; i <= bloque_final; i++)
    {
      uint32_t puntero;
      memcpy(&puntero, blocks_buffer + fcb->f_ip + sizeof(uint32_t) * (i-1), sizeof(uint32_t));
      int cant_a_leer = cantidad_restante > BLOCK_SIZE ? BLOCK_SIZE : cantidad_restante;
      memcpy(data_final + desplazamiento, blocks_buffer + puntero, cant_a_leer);
      desplazamiento += cant_a_leer;
      cantidad_restante -= cant_a_leer;
      log_info(logger_filesystem, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System %d", f_name, i + 1, puntero / BLOCK_SIZE);
      sleep(RETARDO_ACCESO_BLOQUE / 1000);
    }
  }
  free(fcb);

  return data_final;
}