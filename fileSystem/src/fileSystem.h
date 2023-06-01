#ifndef FILESYSTEM
#define FILESYSTEM _H_

#include <utils/utils.h>
#include <stdbool.h>
#include <limits.h>

void load_config(void);
void end_program(int, t_log*, t_config*);

// typedef struct{

//     char* nombre_archivo;
//     int tamanio_archivo;
//     int puntero_directo;
//     int puntero_indirecto;

// } FileSystem_FCB;

 typedef struct{

    int bloque;
    int cantidad_bloque;

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


} FileSystem_config;

FileSystem_config filesystem_config;

t_log * filesystem_logger;
t_config * filesystem_config_file;



#endif /* FILESYSTEM_H_ */

