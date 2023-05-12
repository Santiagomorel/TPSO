#ifndef FILESYSTEM
#define FILESYSTEM _H_

#include <utils/utils.h>


void load_config(void);
void end_program(int, t_log*, t_config*);
typedef struct{

    char* ip_memoria;
    char* path_superbloque;
    char* path_bitmap;
    char* path_bloques;
    char* path_FCB;
    int puerto_memoria;
    int puerto_escucha;
    int retardo_acceso_bloque;

    char* ip_fileSystem;

    int puerto_fileSystem;


} FileSystem_config;

FileSystem_config fileSystem_config;

t_log * fileSystem_logger;
t_config * fileSystem_config_file;

#endif /* FILESYSTEM_H_ */

