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
		run_tests();
		
	}
	else
	{
		// Rutinas de preparación
		levantar_loggers_filesystem();
		levantar_config_filesystem();
		levantar_superbloque();

		socket_memoria = establecer_conexion(IP_MEMORIA, PUERTO_MEMORIA, CONFIG_FILESYSTEM, logger_filesystem);
		//*********************
		// PREPARACIÓN DE BLOQUES - TODO: Hacer logs
		bitmap = levantar_bitmap();
		blocks_buffer = levantar_bloques();

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
		socket_servidor_filesystem = iniciar_servidor(logger_filesystem, PUERTO_ESCUCHA_FILESYSTEM);
		if (socket_servidor_filesystem == -1)
		{
			log_error(logger_filesystem, "No se pudo iniciar el servidor en Filesystem...");
			return EXIT_FAILURE;
		}
		log_info(logger_filesystem, "Filesystem escuchando conexiones...");
		while (server_escuchar(logger_filesystem, socket_servidor_filesystem, (void *)procesar_conexion));

    pthread_t threadDispatch;

    pthread_create(&threadDispatch, NULL, (void *) procesar_conexion, NULL);
    pthread_join(threadDispatch, NULL);
 


		bitarray_destroy(bitmap);
		free(blocks_buffer);
		config_destroy(CONFIG_FILESYSTEM);
		return EXIT_SUCCESS;
	}
}

void levantar_loggers_filesystem()
{
  logger_filesystem = log_create("./log/filesystem.log", "FILESYSTEM", true, LOG_LEVEL_INFO);
}

void levantar_config_filesystem()
{
  CONFIG_FILESYSTEM = config_create("./cfg/filesystem.config");
  IP_MEMORIA = config_get_string_value(CONFIG_FILESYSTEM, "IP_MEMORIA");
  PUERTO_MEMORIA = config_get_string_value(CONFIG_FILESYSTEM, "PUERTO_MEMORIA");
  PUERTO_ESCUCHA_FILESYSTEM = config_get_string_value(CONFIG_FILESYSTEM, "PUERTO_ESCUCHA");
  PATH_SUPERBLOQUE = config_get_string_value(CONFIG_FILESYSTEM, "PATH_SUPERBLOQUE");
  PATH_BITMAP = config_get_string_value(CONFIG_FILESYSTEM, "PATH_BITMAP");
  PATH_BLOQUES = config_get_string_value(CONFIG_FILESYSTEM, "PATH_BLOQUES");
  PATH_FCB = config_get_string_value(CONFIG_FILESYSTEM, "PATH_FCB");
  RETARDO_ACCESO_BLOQUE = config_get_int_value(CONFIG_FILESYSTEM, "RETARDO_ACCESO_BLOQUE");
}

void levantar_superbloque()
{
  CONFIG_SUPERBLOQUE = config_create(PATH_SUPERBLOQUE);
  BLOCK_SIZE = config_get_int_value(CONFIG_SUPERBLOQUE, "BLOCK_SIZE");
  BLOCK_COUNT = config_get_int_value(CONFIG_SUPERBLOQUE, "BLOCK_COUNT");
  config_destroy(CONFIG_SUPERBLOQUE);
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
  char path[46]; // 46 viene de los caracteres de: ./fs/fcb/f_name.config
  strcpy(path, PATH_FCB);
  strcat(path, "/");
  strcat(path, f_name);
  strcat(path, ".config");

  t_config *FCB = config_create(path);
  t_fcb *fcb = malloc(sizeof(t_fcb));
  strncpy(fcb->f_name, f_name, 29); // Si la cadena de origen tiene menos de 29 caracteres, los faltantes se llenan con caracteres nulos
  fcb->f_name[29] = '\0';           // Agrega el carácter nulo al final
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

void establecer_conexion(char * ip_memoria, char* puerto_memoria, t_config* config, t_log* logger){

	
	log_trace(logger, "Inicio como cliente");

	log_trace(logger,"Lei la IP %s , el Puerto Memoria %s ", ip_memoria, puerto_memoria);

	/*----------------------------------------------------------------------------------------------------------------*/

	// Enviamos al servidor el valor de ip como mensaje si es que levanta el cliente
	if((socket_memoria = crear_conexion(ip_memoria, puerto_memoria)) == -1){
		log_trace(logger, "Error al conectar con Memoria. El servidor no esta activo");
        free(socket_memoria);
		exit(-1);
	}else{
		//handshake_cliente(conexion_cpu);
		enviar_mensaje(ip_memoria, socket_memoria);
	}

    recibir_operacion(socket_memoria);
    recibir_mensaje(socket_memoria, logger_filesystem);


}
