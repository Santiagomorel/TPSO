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

//t_list* tabla_segmentos_global;
//esta tabla estara compuesta por segmentos

t_list* tabla_de_procesos;



char* datos;
void* MEMORIA_PRINCIPAL;
int idGlobal;
t_segmento* segmento_compartido;


//KERNEL
void liberar_bitmap_segmento(t_segmento* segmento);
void eliminar_tabla_segmentos(t_list* tabla_segmentos);
void eliminar_proceso(t_proceso* proceso);
void borrar_proceso(int PID);
t_proceso* buscar_proceso(int id_proceso);
t_proceso* borrar_segmento(int PID, int segmento);

//CPU

//Lee el valor de memoria "direc_fisica" y lo almacena en el Registro que luego envia.
void mov_in(int socket_cliente, int direc_fisica, int size);


int iniciarSegmentacion(void);
void iniciar_segmento_0();

// GuardarEnMemoria
t_segmento *guardarElemento(void *elemento, int size);
t_segmento* buscarSegmentoSegunTamanio(int);
t_list* buscarSegmentosDisponibles();
t_segmento* buscarUnLugarLibre(int* base);
void guardarEnMemoria(void *elemento, t_segmento *segmento, int size);
void ocuparMemoria(void *elemento, int base, int size);
t_list* puedenGuardar(t_list* segmentos, int size);

int generarId();

//bitArrays

//asignarBits/Bytes
char* asignarMemoriaBits(int bits);
char* asignarMemoriaBytes(int bytes);
int bitsToBytes(int bits);

//bitMaps
t_bitarray* bitMapSegment;
t_bitarray* unBitmap;

void ocuparBitMap(int base,int size); //ocupa del bitmap global "bitMapSegment"
void liberarBitMap(int base, int size); //libera del bitmap global "bitMapSegment"
int contarEspaciosLibresDesde(t_bitarray* bitmap, int i);
int contarEspaciosOcupadosDesde(t_bitarray*unBitmap, int i);

//criterio de asignacion
t_segmento* elegirSegCriterio(t_list* segmentos, int size);
t_segmento* segmentoBestFit(t_list* segmentos, int size);
t_segmento* segmentoWorstFit(t_list* segmentos, int size); // este "size" puede que no vaya
t_segmento* segmentoMenorTamanio(t_segmento* segmento, t_segmento* otroSegmento);
t_segmento* segmentoMayorTamanio(t_segmento* segmento, t_segmento* otroSegmento);

//Compactacion
void compactacion2();
//el resto de compactacion puede q no vaya
void compactacion();
t_list* buscarSegmentosOcupados();
t_list* copiarContenidoSeg(t_list* segmentosNoCompactados);
void* copiarSegmentacion(t_segmento* unSegmento);
void actualizarCompactacion(t_list* segmentosNoCompactados, t_list* segmentosCompactados);
void actualizarCadaSegmento(t_segmento* segmentoViejo, t_segmento* segmentoNuevo);



//Serializacion y creacion de tablas
void generar_tabla_segmentos();
void agregar_segmento_0(t_list* nueva_tabla_segmentos);
//Serializacion
void enviar_tabla_segmentos(int, int, t_proceso*);
void agregar_tabla_a_paquete(t_paquete*,t_proceso* , t_log*);

int socket_servidor_memoria;
int socket_cliente_memoria_CPU;
int socket_cliente_memoria_FILESYSTEM;
int socket_cliente_memoria_KERNEL;



t_config* memoria_config_file;
t_log* log_memoria;

void load_config(void);

void end_program();

void recibir_kernel(int);
void recibir_cpu(int);
void recibir_fileSystem(int);

//semaforos

sem_t finModulo;
pthread_mutex_t mutexBitMapSegment;
pthread_mutex_t mutexMemoria;
pthread_mutex_t mutexIdGlobal;
pthread_mutex_t listaProcesos;



void iniciar_semaforos();


//listas
void eliminarLista(t_list* lista);
void eliminarAlgo(void* algo);
t_proceso*  crear_proceso_en_memoria(int);


void eliminarTablaDeProcesos();
void eliminarTablaDeSegmentos(t_proceso* proceso);
//void enviar_tabla_segmentos();
//void* serializar_segmento(t_segmento* segmento);
//void* serializar_segmento(void* segmento);
#endif /*MEMORIA_H_*/