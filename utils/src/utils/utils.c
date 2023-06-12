#include "utils.h"

/*      -------------------  Funciones Servidor  -------------------      */

int iniciar_servidor(char *port, t_log *logger)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(IP, port, &hints, &servinfo);
	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo->ai_family,
							 servinfo->ai_socktype,
							 servinfo->ai_protocol);

	log_trace(logger, "Se inicio el server");
	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);
	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);
	log_trace(logger, "Listo para escuchar a mi cliente");

	freeaddrinfo(servinfo);
	return socket_servidor;
}

int esperar_cliente(int socket_servidor, t_log *logger)
{
	// Quitar esta línea cuando hayamos terminado de implementar la funcion

	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL);
	log_info(logger, "Se conecto un cliente!");

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
	{
		return cod_op;
	}
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void *recibir_buffer(int *size, int socket_cliente)
{
	void *buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente, t_log *logger)
{
	int size;
	char *buffer = recibir_buffer(&size, socket_cliente);
	log_trace(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

void recieve_handshake(int socket_cliente)
{
	uint32_t handshake;
	uint32_t resultOk = 0;
	uint32_t resultError = -1;

	recv(socket_cliente, &handshake, sizeof(uint32_t), MSG_WAITALL);
	if (handshake == 1)
		send(socket_cliente, &resultOk, sizeof(uint32_t), NULL);
	else
		send(socket_cliente, &resultError, sizeof(uint32_t), NULL);
}

t_list *recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void *buffer;
	t_list *valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while (desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);
		char *valor = malloc(tamanio);
		memcpy(valor, buffer + desplazamiento, tamanio);
		desplazamiento += tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

/*      -------------------  Funciones Cliente  -------------------      */

void *serializar_paquete(t_paquete *paquete, int bytes)
{
	void *magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;

	return magic;
}

int crear_conexion(char *ip, char *puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int return_value_getaddrinfo = getaddrinfo(ip, puerto, &hints, &server_info);
	// printf("\nEl valor de retorno de la funcion getaddrinfo es %s \n", return_value_getaddrinfo);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	// printf("\nel valor de retorno del socket cliente cuando se genera en crear_conexion es %d \n", socket_cliente);

	if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
	{
		return -1;
	}

	freeaddrinfo(server_info);

	return socket_cliente;
}

void enviar_mensaje(char *mensaje, int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	int return_value_send = send(socket_cliente, a_enviar, bytes, 0);
	// printf("\nEl valor de retorno de send en la funcion enviar mensaje es %d\n", return_value_send);
	free(a_enviar);
	eliminar_paquete(paquete);
}

void crear_buffer(t_paquete *paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete *crear_super_paquete(void)
{
	// me falta un malloc!
	t_paquete *paquete;

	// descomentar despues de arreglar
	// paquete->codigo_operacion = PAQUETE;
	// crear_buffer(paquete);
	return paquete;
}

t_paquete *crear_paquete(void)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE;
	crear_buffer(paquete);
	return paquete;
}

t_paquete *crear_paquete_op_code(op_code codigo_op)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo_op;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void agregar_entero_a_paquete(t_paquete *paquete, int x)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size, &x, sizeof(int));
	paquete->buffer->size += sizeof(int);
}


void agregar_array_string_a_paquete(t_paquete* paquete, char** arr)
{
	int size = string_array_size(arr);
	agregar_entero_a_paquete(paquete, size);
	for (int i = 0; i < size; i++)
		agregar_a_paquete(paquete, arr[i], string_length(arr[i]) + 1);
}

void agregar_registros_a_paquete(t_paquete *paquete, t_registro *registro)
{
	int size = sizeof(t_registro);
	agregar_entero_a_paquete(paquete, size);
	agregar_a_paquete(paquete, registro->AX, 4);
	agregar_a_paquete(paquete, registro->BX, 4);
	agregar_a_paquete(paquete, registro->CX, 4);
	agregar_a_paquete(paquete, registro->DX, 4);
	agregar_a_paquete(paquete, registro->EAX, 8);
	agregar_a_paquete(paquete, registro->EBX, 8);
	agregar_a_paquete(paquete, registro->ECX, 8);
	agregar_a_paquete(paquete, registro->EDX, 8);
	agregar_a_paquete(paquete, registro->RAX, 16);
	agregar_a_paquete(paquete, registro->RBX, 16);
	agregar_a_paquete(paquete, registro->RCX, 16);
	agregar_a_paquete(paquete, registro->RDX, 16);
}

void enviar_paquete(t_paquete *paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2 * sizeof(int);
	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete *paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}

void send_handshake(int socket_cliente)
{
	uint32_t handshake = 1;
	uint32_t result;

	send(socket_cliente, &handshake, sizeof(uint32_t), NULL);
	recv(socket_cliente, &result, sizeof(uint32_t), MSG_WAITALL);
	printf("El valor de retorno del handshake es: %d", result);
}

/*      -------------------  Funciones Configuracion Inicial  -------------------      */

t_config *init_config(char *config_path)
{
	t_config *new_config;
	if ((new_config = config_create(config_path)) == NULL)
	{
		printf("No se pudo cargar la configuracion");
		exit(1);
	}
	return new_config;
}

t_log *init_logger(char *file, char *process_name, bool is_active_console, t_log_level level)
{
	t_log *new_logger;
	if ((new_logger = log_create(file, process_name, is_active_console, level)) == NULL)
	{
		printf("No se puede iniciar el logger\n");
		exit(1);
	}
	return new_logger;
}

/*      -------------------  Funciones de Serializacion/Deserializacion -------------------      */

int leer_entero(char *buffer, int *desplazamiento) // Lee un entero en base a un buffer y un desplazamiento, ambos se pasan por referencia
{
	int ret;
	memcpy(&ret, buffer + (*desplazamiento), sizeof(int)); // copia dentro de ret lo que tiene el buffer con un size de int
	(*desplazamiento) += sizeof(int);
	return ret;
}

t_list *leer_segmento(char *buffer, int *desplazamiento) // Lee un entero en base a un buffer y un desplazamiento, ambos se pasan por referencia
{
	t_list *ret;
	memcpy(&ret, buffer + (*desplazamiento), sizeof(t_list));
	(*desplazamiento) += sizeof(int);
	return ret;
} // revisar si se descerializa bien

float leer_float(char *buffer, int *desplazamiento) // Lee un float en base a un buffer y un desplazamiento, ambos se pasan por referencia
{
	float ret;
	memcpy(&ret, buffer + (*desplazamiento), sizeof(float));
	(*desplazamiento) += sizeof(float);
	return ret;
}

char *leer_string(char *buffer, int *desplazamiento) // Lee un string en base a un buffer y un desplazamiento, ambos se pasan por referencia
{
	int tamanio = leer_entero(buffer, desplazamiento);
	//printf("allocating / copying %d \n", tamanio);

	char *valor = malloc(tamanio);
	memcpy(valor, buffer + (*desplazamiento), tamanio);
	(*desplazamiento) += tamanio;

	return valor;
}

char **leer_string_array(char *buffer, int *desp)
{
	int length = leer_entero(buffer, desp);
	char **arr = malloc((length + 1) * sizeof(char *));

	for (int i = 0; i < length; i++)
	{
		arr[i] = leer_string(buffer, desp);
	}
	arr[length] = NULL;

	return arr;
}

t_registro * leer_registros(char* buffer, int * desp) {
	int tamanio = leer_entero(buffer, desp);
	t_registro * retorno = malloc(tamanio);
	leer_entero(buffer, desp);
	memcpy(retorno->AX, buffer + (*desp), 4);
	(*desp) += 4;
	leer_entero(buffer, desp);
	memcpy(retorno->BX, buffer + (*desp), 4);
	(*desp) += 4;
	leer_entero(buffer, desp);
	memcpy(retorno->CX, buffer + (*desp), 4);
	(*desp) += 4;
	leer_entero(buffer, desp);
	memcpy(retorno->DX, buffer + (*desp), 4);
	(*desp) += 4;
	leer_entero(buffer, desp);
	memcpy(retorno->EAX, buffer + (*desp), 8);
	(*desp) += 8;
	leer_entero(buffer, desp);
	memcpy(retorno->EBX, buffer + (*desp), 8);
	(*desp) += 8;
	leer_entero(buffer, desp);
	memcpy(retorno->ECX, buffer + (*desp), 8);
	(*desp) += 8;
	leer_entero(buffer, desp);
	memcpy(retorno->EDX, buffer + (*desp), 8);
	(*desp) += 8;
	leer_entero(buffer, desp);
	memcpy(retorno->RAX, buffer + (*desp), 16);
	(*desp) += 16;
	leer_entero(buffer, desp);
	memcpy(retorno->RBX, buffer + (*desp), 16);
	(*desp) += 16;
	leer_entero(buffer, desp);
	memcpy(retorno->RCX, buffer + (*desp), 16);
	(*desp) += 16;
	leer_entero(buffer, desp);
	memcpy(retorno->RDX, buffer + (*desp), 16);
	(*desp) += 16;
	return retorno;
}

void loggear_pcb(t_pcb *pcb, t_log *logger)
{
	int i = 0;

	log_trace(logger, "id %d", pcb->id);
	loggear_estado(logger, pcb->estado_actual);
	for (i = 0; i < string_array_size(pcb->instrucciones); i++)
	{
		log_trace(logger, "instruccion Linea %d: %s", i, pcb->instrucciones[i]);
	}
	log_trace(logger, "program counter %d", pcb->program_counter);
	// log_trace(logger, "tabla de pags %d", pcb->tabla_paginas);
	log_trace(logger, "estimacion rafaga actual %f", pcb->estimacion_rafaga);
	log_trace(logger, "socket_cliente_consola %d", pcb->socket_consola);
}

void loggear_estado(t_log *logger, int estado)
{
	char *string_estado;

	switch (estado)
	{
	case NEW:
		string_estado = string_duplicate("NEW");
		break;
	case READY:
		string_estado = string_duplicate("REDY");
		break;
	case BLOCKED:
		string_estado = string_duplicate("BLOCKED");
		break;
	// case BLOCKED_READY:
	// 	string_estado = string_duplicate("BLOCKED_READY");
	// 	break;
	case RUNNING:
		string_estado = string_duplicate("RUNNING");
		break;
	case EXIT:
		string_estado = string_duplicate("EXIT");
		break;
	}

	log_trace(logger, "estado %d (%s)", estado, string_estado);
	free(string_estado);
}

t_list *recibir_paquete_segmento(int socket)
{ // usar desp de recibir el COD_OP

	int size;
	char *buffer;
	int desp = 0;

	buffer = recibir_buffer(&size, socket);

	t_list *segmento = leer_segmento(buffer, &desp);

	free(buffer);
	return segmento;
}

contexto_ejecucion *recibir_ce(int socket_kernel)
{
	contexto_ejecucion *nuevoCe = malloc(sizeof(contexto_ejecucion));
	int size = 0;
	char *buffer;
	int desp = 0;

	buffer = recibir_buffer(&size, socket_kernel);

	nuevoCe->id = leer_entero(buffer, &desp);
	nuevoCe->instrucciones = leer_string_array(buffer, &desp); // hay que liberar antes de perder la referencia
	nuevoCe->program_counter = leer_entero(buffer, &desp);
	nuevoCe->registros_cpu = leer_registros(buffer, &desp); // hay que liberar antes de perder la referencia
	// nuevoCe->tabla_segmentos = leer_tabla_segmentos(buffer, &desp);
	free(buffer);
	return nuevoCe;
}

t_paquete *agregar_tabla_segmentos_a_paquete(t_paquete *paquete, t_list *tabla)
{ // le saque el tamanio tabla porque creo que con sizeof t_list se soluciona
}

void enviar_ce(int conexion, contexto_ejecucion *ce, int codOP, t_log *logger)
{
	t_paquete *paquete = crear_paquete_op_code(codOP);

	agregar_ce_a_paquete(paquete, ce, logger);

	enviar_paquete(paquete, conexion);

	eliminar_paquete(paquete);
}

void agregar_ce_a_paquete(t_paquete *paquete, contexto_ejecucion *ce, t_log *logger)
{
	log_trace(logger, "antes de agregar entero");
	agregar_entero_a_paquete(paquete, ce->id);
	log_trace(logger, "despues de agregar entero");
	agregar_array_string_a_paquete(paquete, ce->instrucciones);
	log_trace(logger, "despues de agregar array de strings");
	agregar_entero_a_paquete(paquete, ce->program_counter);
	log_trace(logger, "despues de agregar program counter");
	agregar_registros_a_paquete(paquete, ce->registros_cpu);
	log_trace(logger, "despues de agregar registros"); // crear la funcion para mandar los registros.

	// agregar_tabla_segmentos_a_paquete(paquete, ce->tabla_segmentos); MODIFICAR
}

void imprimir_ce(contexto_ejecucion* ce, t_log* logger) {
	log_trace(logger, "El id del CE es %d", ce->id);
	for (int i = 0; i < string_array_size(ce->instrucciones); i++) {
        log_trace(logger, "Instruccion %d del CE es %s", i, ce->instrucciones[i]);
    }
	log_trace(logger, "El PC del CE es %d", ce->program_counter);
	imprimir_registros(ce->registros_cpu, logger);
	//imprimir_tabla_segmentos
}

void imprimir_registros(t_registro* registros , t_log* logger) {
	log_trace(logger, "El registro AX es %.*s", 4,registros->AX);
	log_trace(logger, "El registro BX es %.*s", 4,registros->BX);
	log_trace(logger, "El registro CX es %.*s", 4,registros->CX);
	log_trace(logger, "El registro DX es %.*s", 4,registros->DX);
	log_trace(logger, "El registro EAX es %.*s",8,registros->EAX);
	log_trace(logger, "El registro EBX es %.*s",8,registros->EBX);
	log_trace(logger, "El registro ECX es %.*s",8,registros->ECX);
	log_trace(logger, "El registro EDX es %.*s",8,registros->EDX);
	log_trace(logger, "El registro RAX es %.*s",16,registros->RAX);
	log_trace(logger, "El registro RBX es %.*s",16,registros->RBX);
	log_trace(logger, "El registro RCX es %.*s",16,registros->RCX);
	log_trace(logger, "El registro RDX es %.*s",16,registros->RDX);
}

void liberar_ce(contexto_ejecucion* ce){
	//free(ce->id); // seg fault por tratar de hacer un free a un int
	free(ce->instrucciones); // probablemente tengamos tambien que liberar las instrucciones 1 a 1 (me da paja)
	//free(ce->program_counter); // seg fault por tratar de hacer un free a un int
	free(ce->registros_cpu);
	//liberar_tabla(ce->tabla_segmentos); falta hacer
}

char* obtenerCodOP(int cop){
	switch (cop)
	{
	case SUCCESS:
		return "SUCCESS";
		break;
	case SEG_FAULT:
		return "SEG_FAULT";
		break;
	case OUT_OF_MEMORY:
		return "OUT_OF_MEMORY";
		break;
	default:
		break;
	}
}