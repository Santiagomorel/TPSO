#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <utils/utils.h>



void load_config(void);
void end_program(int, t_log*, t_config*);

// typedef struct{

//     char* nombre_archivo;
//     int tamanio_archivo;
//     int puntero_directo;
//     int puntero_indirecto;

// } FileSystem_FCB;

 typedef struct{

    int block_size;
    int block_count;

} Filesystem_superbloque;

typedef struct{

    char* ip_memoria;
    char* path_superbloque;
    char* path_bitmap;
    char* path_bloques;
    char* path_FCB;
    int puerto_memoria;
    int puerto_escucha;
    int retardo_acceso_bloque;

    char* ip_filesystem;

    int puerto_filesystem;


} Filesystem_config;

Filesystem_config filesystem_config;

t_log * filesystem_logger;
t_config * filesystem_config_file;

Filesystem_superbloque* armar_superbloque();
t_bitarray * armar_bitmap();
t_list* armar_bloques();

int socket_cliente_filesystem_kernel;
int socket_servidor_filesystem;
int conexion_memoria;

void recibir_kernel(int);




#endif /* FILESYSTEM_H_ */

