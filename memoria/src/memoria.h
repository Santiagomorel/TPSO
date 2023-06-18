#ifndef MEMORIA_H_
#define MEMORIA_H_


#include <utils/utils.h>
//typedef enum{
//    KERNEL,
//    CPU,
//    FILESYSTEM,
//    OTRO
//}cod_mod;

//cod_mod recibir_handshake(int socket_cliente){
//    cod_mod rta_handshake;
//
//    recv(socket_cliente, &rta_handshake, sizeof(cod_mod), MSG_WAITALL);
//    return rta_handshake;
//}

typedef struct{

    char* puerto_escucha;
    int tam_memoria;
    int tam_segmento_0;
    int cant_segmentos;
    int retardo_memoria;
    int retardo_compactacion;
    char* algoritmo_asignacion;

} Memoria_config;
Memoria_config memoria_config;

typedef struct{
	int id;
	t_list* tabla_segmentos;
}t_proceso;

//esta mal (casi seguro)
typedef struct{
    int init_direc;
    int tamanio;
}t_hueco;

char* datos;




int iniciarSegmentacion(void);

// GuardarEnMemoria
t_segmento *guardarElemento(void *elemento, int size);
t_segmento* buscarSegmentoSegunTamanio(int size);
void guardarEnMemoria(void *elemento, t_segmento *segmento, int size);
void ocuparMemoria(void *elemento, int base, int size);
t_list* puedenGuardar(t_list* segmentos, int size);

//bitArrays

//asignarBits/Bytes
char* asignarMemoriaBits(int bits);
char* asignarMemoriaBytes(int bytes);
int bitsToBytes(int bits);

//bitMaps
t_bitarray* bitMapSegment;
t_bitarray* unBitmap;

void ocuparBitMap(t_bitarray* segmentoBitMap, int base,int size);
void liberarBitMap(t_bitarray* segmentoBitMap, int base, int size);
int contarEspaciosLibresDesde(t_bitarray* bitmap, int i);
int contarEspaciosOcupadosDesde(t_bitarray*unBitmap, int i);

//criterio de asignacion
t_segmento* elegirSegCriterio(t_list* segmentos, int size);

t_list* generar_lista_huecos();
void generar_tabla_segmentos();
t_segmento* crear_segmento(int id_seg, int base, int tamanio);
// void enviar_tabla_segmentos(int, t_log*);
//Serializacion
void enviar_tabla_segmentos(int, int, t_log*);
void agregar_tabla_a_paquete(t_paquete*,t_proceso* , t_log*);
t_proceso * recibir_tabla_segmentos(int , t_log*);
t_list* leer_tabla_segmentos(char*,int* );

int socket_servidor_memoria;
int socket_cliente_memoria_CPU;
int socket_cliente_memoria_FILESYSTEM;
int socket_cliente_memoria_KERNEL;

void* MEMORIA_PRINCIPAL;

t_config* memoria_config_file;
t_log* log_memoria;

void load_config(void);

void end_program(int ,t_log* ,t_config* );

void recibir_kernel(int);
void recibir_cpu(int);
void recibir_fileSystem(int);

//semaforos

sem_t finModulo;
pthread_mutex_t mutexBitMapSegment;
pthread_mutex_t mutexMemoria;

void iniciar_semaforos();


//listas
void eliminarLista(t_list* lista);
void eliminarAlgo(void* algo);

//void enviar_tabla_segmentos();
//void* serializar_segmento(t_segmento* segmento);
//void* serializar_segmento(void* segmento);
#endif /*MEMORIA_H_*/


