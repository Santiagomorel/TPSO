#include "utils_start.h"

t_config* init_config(char * config_path)
{
	t_config* new_config;
	if((new_config = config_create(config_path)) == NULL){
		printf("No se pudo cargar la configuracion");
		exit(1);	
    }
	return new_config;
}

t_log* init_logger(char *file, char *process_name, bool is_active_console, t_log_level level)
{
	t_log* new_logger;
	 if((new_logger = log_create(file, process_name, is_active_console, level)) == NULL){
		printf("No se puede iniciar el logger\n");
		exit(1);
	 }
	return new_logger;
}