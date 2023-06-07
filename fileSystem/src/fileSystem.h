#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <utils/utils.h>



void load_config(void);
void end_program(int, t_log*, t_config*);

typedef enum{
    
    MENSAJE_G,
    F_OPEN,
	F_CLOSE,
	F_SEEK,
	F_READ,
	F_WRITE,
	F_TRUNCATE,
};

// typedef struct{

//     char* nombre_archivo;
//     int tamanio_archivo;
//     int puntero_directo;
//     int puntero_indirecto;

// } FileSystem_FCB;

 typedef struct{

    int block_size;
    int block_count;

} t_superbloque;

typedef struct{

    char* ip_memoria;
    char* path_superbloque;
    char* path_bitmap;
    char* path_bloques;
    char* path_FCB;
    char* puerto_memoria;
    char* puerto_escucha;
    char* retardo_acceso_bloque;

} Filesystem_config;

Filesystem_config filesystem_config;

t_log * filesystem_logger;
t_config * filesystem_config_file;

void armar_superbloque();
void armar_bitmap();
void armar_bloques();

int socket_cliente_filesystem_kernel;
int socket_servidor_filesystem;
int conexion_memoria;

void recibir_kernel(int);


t_superbloque* superbloque;
t_bitarray* bitmap;
t_list* archivo_bloques;

#endif /* FILESYSTEM_H_ */

