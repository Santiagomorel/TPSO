#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_


#include <utils/utils.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <commons/bitarray.h>

typedef struct
{
  char f_name[100]; // Nombre del archivo
  uint32_t f_size; // Tamaño del archivo en bytes del archivo
  uint32_t f_dp;   // Puntero directo al primer bloque de datos del archivo
  uint32_t f_ip;   // Puntero indirecto al bloque que contiene los punteros a los siguientes bloques del archivo
} t_fcb;

t_log *logger_filesystem;


t_config *CONFIG_FILESYSTEM;
char *IP_MEMORIA;
char *PUERTO_MEMORIA;
char *PUERTO_ESCUCHA_FILESYSTEM;
char *PATH_SUPERBLOQUE;
char *PATH_BITMAP;
char *PATH_BLOQUES;
char *PATH_FCB;
int RETARDO_ACCESO_BLOQUE;


t_config *CONFIG_SUPERBLOQUE;
int BLOCK_SIZE;
int BLOCK_COUNT;

t_bitarray *bitmap;
char* blocks_buffer;

int socket_memoria;
int socket_servidor_filesystem;
int socket_fs;

void levantar_loggers_filesystem();
void levantar_config_filesystem();
void levantar_superbloque();
t_bitarray *levantar_bitmap();
void ocupar_bloque(int numero_bloque);
void desocupar_bloque(int numero_bloque);
char *levantar_bloques();
char *leer_bloque(uint32_t puntero_a_bloque);
void modificar_bloque(uint32_t puntero_a_bloque, char *bloque_nuevo);
t_fcb *levantar_fcb(char* f_name);
int calcular_bloques_por_size(uint32_t size);
int encontrar_bloque_libre();
void asignar_bloque_directo(t_fcb *fcb);
void asignar_bloque_indirecto(t_fcb *fcb);
void asignar_bloque_al_bloque_indirecto(t_fcb *fcb, int bloques_ya_asignados);
void remover_puntero_de_bloque_indirecto(t_fcb *fcb, int bloques_utiles);
void establecer_conexion(t_log*);


/******************COMUNICACION******************/
void procesar_conexion();

/******************CORE******************/
int abrir_archivo(char* f_name);
void crear_archivo(char* f_name);
void truncar_archivo(char* f_name, uint32_t new_size);
void eferrait(char* f_name, uint32_t offset, uint32_t size, char* data);
void* eferrid(char* f_name, uint32_t offset, uint32_t cantidad);
#endif