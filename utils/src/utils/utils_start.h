#ifndef UTILS_START_H_
#define UTILS_START_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>
#include<commons/config.h>


t_config* init_config(char * config_path);

t_log* init_logger(char *file, char *process_name, bool is_active_console, t_log_level level);
#endif /* UTILS_START_H_ */