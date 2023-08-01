#ifndef MEMORIA_H_
#define MEMORIA_H_


#include <utils/utils.h>

typedef enum {
    FIRST,
    BEST,
    WORST
} t_algo_asig;

typedef struct{

    char* puerto_escucha;
    uint32_t tam_memoria;
    uint32_t tam_segmento_0;
    uint32_t cant_segmentos;
    uint32_t retardo_memoria;
    uint32_t retardo_compactacion;
    t_algo_asig algoritmo_asignacion;

} Memoria_config;
Memoria_config memoria_config;

t_config* memoria_config_file;
t_log* log_memoria;
/////
/////comienza memoria.h juanpi
/////

sem_t finModulo;

int socket_servidor_memoria;
int socket_cliente_memoria_CPU;
int socket_cliente_memoria_FILESYSTEM;
int socket_cliente_memoria_KERNEL;

void recibir_kernel(int SOCKET_CLIENTE_KERNEL);
void recibir_cpu(int SOCKET_CLIENTE_CPU);
void recibir_fileSystem(int SOCKET_CLIENTE_FILESYSTEM);


void* ESPACIO_USUARIO;
uint32_t ESPACIO_LIBRE_TOTAL;
t_list* LISTA_ESPACIOS_LIBRES;
t_list* LISTA_GLOBAL_SEGMENTOS;
pthread_mutex_t mutex_memoria;

typedef struct {
    uint32_t base;
    uint32_t limite;
} t_esp; // Para marcar un hueco de la memoria

//comunicacion
void procesar_conexion(void *void_args);
//



void levantar_estructuras_administrativas();
void crear_segmento_0();
bool comparador_base(void* data1, void* data2);
bool comparador_base_segmento(void* data1, void* data2);
void print_lista_esp(t_list* lista);
void print_lista_segmentos();
void* crear_tabla_segmentos();
int buscar_espacio_libre(uint32_t tam);
op_code crear_segmento_memoria(uint32_t tam, uint32_t* base_resultante);
bool son_contiguos(t_esp* esp1, t_esp* esp2);
int buscar_segmento_por_base(uint32_t base);
void borrar_segmento(uint32_t base, uint32_t limite);
void escribir(uint32_t dir_fisca, void* data, uint32_t size);
char* leer(uint32_t dir_fisca , uint32_t size);
void compactar();
/////
/////termina memoria.h juanpi
/////


/*
//KERNEL
void liberar_bitmap_segmento(t_segmento* segmento);
void eliminar_tabla_segmentos(t_list* tabla_segmentos);
void eliminar_proceso(t_proceso* proceso);
void borrar_proceso(int PID);
t_proceso* buscar_proceso(int id_proceso);
t_proceso* buscar_proceso_aux(int, t_list*);
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
char* leer(uint32_t dir_fisca , uint32_t size);
void escribir(uint32_t dir_fisca, void* data, uint32_t size);

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

void compactacion();
t_list* buscarSegmentosOcupados();
t_list* copiarContenidoSeg(t_list* segmentosNoCompactados);
void* copiarSegmentacion(t_segmento* unSegmento);
void actualizarCompactacion(t_list* segmentosNoCompactados, t_list* segmentosCompactados);
void actualizarCadaSegmento(t_segmento* segmentoViejo, t_segmento* segmentoNuevo);



//Serializacion y creacion de tablas
void generar_tabla_segmentos();
void agregar_segmento_0(t_list*);
//Serializacion
void enviar_tabla_segmentos(int, int, t_proceso*);
void agregar_tabla_a_paquete(t_paquete*,t_proceso* , t_log*);

int socket_servidor_memoria;
int socket_cliente_memoria_CPU;
int socket_cliente_memoria_FILESYSTEM;
int socket_cliente_memoria_KERNEL;





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
pthread_mutex_t mutexUnicaEjecucion;



void iniciar_semaforos();


//listas
void eliminarLista(t_list* lista);
void eliminarAlgo(void* algo);
t_proceso*  crear_proceso_en_memoria(int);


void eliminarTablaDeProcesos();
void eliminarTablaDeSegmentos(t_proceso* proceso);

t_list* adaptar_TDP_salida();
//void enviar_tabla_segmentos();
//void* serializar_segmento(t_segmento* segmento);
//void* serializar_segmento(void* segmento);
*/
#endif /*MEMORIA_H_*/