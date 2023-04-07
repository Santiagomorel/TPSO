#include "utils_config.h"

t_config* init_config(char * config_path)
{
	t_config* new_config;
	if((new_config = config_create(config_path)) == NULL){
		printf("No pude leer la config\n");
		exit(2);	
    }
	return new_config;
}