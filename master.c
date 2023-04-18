// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "masterAndSlave.h"  /* importa variables de entorno librerias, constantes y funciones */

/* 
 * Master :
 * Recibe los directorios de los archivos cuyos md5 se desea calcular ( no preocuparse por permisos o directorios invalidos )
 * Inicia los procesos esclavos ( cantidad decidida arbitrariamente )
 * Gestiona la cola de trabajo de los esclavos
 * Recibe y almacena el resultado de cada esclavo en un buffer por orden de llegada
 * Al ser inicializado debe esperar la aparición del proceso Vista, caso positivo debe compartir el buffer con dicho proceso
 * Guarda el resultado en un archivos results.txt
 */


int createResultsFile();
void createSlavesPipes( int qSlaves , int slavesReadPipe[] , int masterWritePipe[] , int slavesWritePipe[] , int masterReadPipe[] );
void writeNumberOfFiles( int argc );
void setSelectArray( fd_set * readfds , int qSlaves , int masterReadPipe[] );
void spawnSlaves( int qSlaves , int slavesReadPipe[] , int slavesWritePipe[] ) ;
void loadFileName( char * files[] , int posFiles , int * whereToWrite , int posFd );



int main( int argc , char *argv[] )
{
    /* Creacion de archivo de resultados */
    int fdResults = createResultsFile();
    

    /* variables de ciclos */
    int i, j, n;

    /* Calculo las subdivisiones */
    int undigestedFiles = argc - 1;                                                     /* archivos que faltan procesar            */     
    int qSlaves = ( int ) ceil( ( double ) undigestedFiles/50.0 );                      /* cantidad de esclavos                    */     
    int posNextFile = 1;                                                                /* posicion del proximo archivo a procesar */ 
    int initialLoad = ( int ) ceil( undigestedFiles/50.0 );                             /* factor de carga inicial                 */    

    /* Arrays de esclavos */
    int slavesReadPipe[ qSlaves ];                                                      /*  fd del cual cada esclavo lee el archivo que debe digerir   */                              
    int slavesWritePipe[ qSlaves ];                                                     /*  fd donde escribe el resultado de la digestion              */                   
    int masterReadPipe[ qSlaves ];                                                      /*  fd del cual el master lee el resultado de la digestion     */                            
    int masterWritePipe[ qSlaves ];                                                     /*  fd donde el master escribe el archivo a digerir            */                     
    char initiallyLoaded[ qSlaves ];                                                    /*  boolean de si fue cargada o no                             */    
    memset( initiallyLoaded , 0 , sizeof( initiallyLoaded ) );
    char slaveStates[ qSlaves ];                                                        /*  cantidad de trabajo pendientes                             */    
    memset( slaveStates , 0 , sizeof( slaveStates ) );

    /* Creacion de tuberías */
    createSlavesPipes(qSlaves, slavesReadPipe, masterWritePipe, slavesWritePipe, masterReadPipe);

    /* Manejo de Semaforos y Shared Memory */
    shme_t shmPtr = shmMake( SHM_NAME , SHM_SIZE );                                       
    sema_t sem = semCreate( SEM_NAME );

    sleep( 5 );                                                                         /*  Se espera a la ejecucion del proceso vista */                       

    /* Retorno de la informacion requerida por el Vista por STDOUT (cantidad de archivos a hashear) */
    writeNumberOfFiles( argc );

    /* Creacion y Seteo del Select */
    fd_set readfds;                                                                     /*  array con los fds a monitorear    */
    setSelectArray( &readfds , qSlaves , masterReadPipe );                   

    /* Creacion de los Slaves */
    spawnSlaves( qSlaves , slavesReadPipe , slavesWritePipe );

    
    /* Monitoreo y Escritura sobre results.txt de forma dinamica */
    int nfdSlave = 4 * qSlaves + 3;
    int offset = 0;
    struct timeval tv;                                                                  /* por consejo de internet para que no se bloquee el select */
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while( undigestedFiles > 0 ){
        /* Cargado inicial de los buffers personalizados para cada slave con la carga inicial */
        for ( n = 0 ; n < qSlaves && posNextFile <= initialLoad * qSlaves  ; n++){
            if ( initiallyLoaded[ n ] == 0 && slaveStates[ n ] == 0 ){
                for ( j = posNextFile ; j < ( posNextFile + initialLoad ) ; j++ ){
                
                    loadFileName( argv , j , masterWritePipe , n );
                    slaveStates[ n ]++;
                    
                }
                posNextFile += initialLoad;
                initiallyLoaded[ n ] = 1;
            }
        }

        /* Monitoreo de Fds para obtener una lectura no bloqueante */
        int qFdsToRead = select( nfdSlave , &readfds , NULL , NULL , &tv );
        for ( n = 0; n < qSlaves && qFdsToRead != 0 ; n++ ){
            if ( FD_ISSET( masterReadPipe[ n ] , &readfds ) ){

                /* lectura del buffer del slave que escribio su resultado */
                char resultBuffer[ MAX_BUFFER_SIZE ];
                int bytesRead;
                bytesRead = read( masterReadPipe[ n ] , resultBuffer , MAX_BUFFER_SIZE );
                if ( bytesRead == -1 ) {
                    handle_error( "read failed master" );
                }

                /* escritura en el archivo results.txt */
                int bytesWritten = write( fdResults , resultBuffer , bytesRead );
                if( bytesWritten == -1 ){
                    handle_error( "write failed master" );
                }
                
                /* escritura en la memoria compartida */
                strcpy( shmPtr.address + offset , resultBuffer );                      /* mandamos la shared  */
                offset += strlen( resultBuffer ) ;

                if( sem_post( sem.access ) == - 1 ){                                      /* levantamos el semaforo asi no se bloquea */
                    handle_error( "sem_post failed master" );
                } 
                
                /* movimiento de variables */
                slaveStates[ n ]--;
                qFdsToRead--;
                undigestedFiles--;

                /* caso donde un slave consumio los archivos que se le dieron, entonces se le carga un archivo nuevo */
                int isAvailable = slaveStates[n] == 0 && posNextFile < argc;
                if ( isAvailable ){
                    loadFileName( argv , posNextFile , masterWritePipe , n );
                    slaveStates[ n ]++;
                    posNextFile++;
                }
    
            }
        }
        /* Reiniciado del Select, si un slave ya no puede trabajar porque no hay mas archivos se retira su fd del select */
        FD_ZERO( &readfds );
        for ( n = 0 ; n < qSlaves ; n++ ){
            if ( slaveStates[ n ] == 0 ){
                FD_SET( masterReadPipe[ n ], &readfds );
                FD_CLR( masterReadPipe[ n ], &readfds );
            }
            else{
                FD_SET( masterReadPipe[ n ], &readfds );
            }
        }
    }

    /* Cerrado de Fd del master, shared memory y semaforos */
    int slaveFdsToClose = 4 * qSlaves + 3;
    for ( i = 3 ; i < slaveFdsToClose ; i++ ){
        if ( close( i ) == - 1){
            handle_error( "close failed master" );
        }
    }
    
    /* Unlink shared memory */
    shmDestroy( &shmPtr );

    /* Unlink semaforo */
    semFinish( &sem );
    

    return 0;
}




/**
 * Función que concatena dos cadenas de caracteres en un buffer, sin modificar la cadena original de destino.
 * @param str1 La primera cadena de caracteres a concatenar.
 * @param str2 La segunda cadena de caracteres a concatenar.
 * @param buffer Puntero al buffer donde se almacenará la cadena resultante de la concatenación.
 * @param bufferSize Tamaño del buffer.
*/
static void concat( const char * str1 , const char* str2 , char* buffer , size_t bufferSize ) {
    size_t len1 = strlen( str1 );
    size_t len2 = strlen( str2 );
    size_t requiredSize = len1 + len2 + 1;
    if ( requiredSize > bufferSize ) {
        handle_error( "buffer too small" );
    }
    snprintf( buffer , bufferSize , "%s%s" , str1 , str2 );
}

/**
 * Carga en el pipe compartido entre master y slave el nombre del archivo requerido.
 * @param files Arreglo de strings con los nombres de los archivos.
 * @param posFiles Posición en el arreglo de strings del archivo a cargar.
 * @param whereToWrite Arreglo de file descriptors para escribir en los pipes.
 * @param posFd Posición en el arreglo de file descriptors del pipe a utilizar.
 */
void loadFileName( char * files[] , int posFiles , int * whereToWrite , int posFd ){
    char buffer[ MAX_PATH_SIZE ];
    concat( files[ posFiles ] , "\n" , buffer , MAX_PATH_SIZE );
    if( write( whereToWrite[ posFd ] , buffer , strlen( buffer ) ) == -1 ){
        handle_error( "write failed" );
    }
}

/**
 * Crea el archivo results.txt para almacenar los resultados de los slaves.
 * @return File descriptor del archivo results.txt.
 */
int createResultsFile(){
    int fdResults = open( "results.txt" , O_WRONLY | O_CREAT | O_TRUNC , 0666 );
    if ( fdResults < 0 ){
        handle_error( "open results.txt" );
    }
    return fdResults;
}


/**
 * createSlavePipes - Crea los pipes necesarios para comunicar procesos esclavos con el proceso maestro.
 * @param qSlaves Cantidad de procesos esclavos.
 * @param slavesReadPipe Arreglo para almacenar los pipes de lectura de los procesos esclavos.
 * @param masterWritePipe Arreglo para almacenar los pipes de escritura del proceso maestro.
 * @param slavesWritePipe Arreglo para almacenar los pipes de escritura de los procesos esclavos.
 * @param masterReadPipe Arreglo para almacenar los pipes de lectura del proceso maestro.
*/
void createSlavesPipes(int qSlaves, int slavesReadPipe[], int masterWritePipe[], int slavesWritePipe[], int masterReadPipe[]) {
    int mtosPipe[2]; /* master to slave Pipe */
    int stomPipe[2]; /* slave to master Pipe */
    int n;

    /* Create pipes for each slave process */
    for (n = 0; n < qSlaves; n++) {
        /* Create master to slave pipe */
        if (pipe(mtosPipe) < 0) {
            handle_error("Error creating master to slave pipe");
        }
        slavesReadPipe[n] = mtosPipe[0];
        masterWritePipe[n] = mtosPipe[1];

        /* Create slave to master pipe */
        if (pipe(stomPipe) < 0) {
            handle_error("Error creating slave to master pipe");
        }
        masterReadPipe[n] = stomPipe[0];
        slavesWritePipe[n] = stomPipe[1];
    }
}


/**
 * Configura el conjunto de descriptores de archivo para monitorear con select() en los pipes de lectura del proceso principal
 * @param readfds - un puntero al arreglo fd_set que será llenado con los pipes de lectura del proceso principal
 * @param qSlaves - la cantidad de procesos esclavos
 * @param masterReadPipe - un arreglo que contiene los descriptores de archivo de los pipes de lectura del proceso principal para cada proceso esclavo
 */
void setSelectArray( fd_set * readfds , int qSlaves , int masterReadPipe[] ) {
    int n;
    FD_ZERO( readfds );                                                                 /* seteo en 0 */  
    for ( n = 0 ; n < qSlaves ; n++ ) {
        FD_SET( masterReadPipe[ n ] , readfds );                                        /* seteo los fds a monitorear */
    }
}


/**
 * Función que escribe en el stdout del proceso master la cantidad de archivos que recibio, en caso de ejecutar vista por medio
 * de un pipe, el stdout se toma para el stdin del proceso vista.
 * @param argc cantidad de argumentos pasados por línea de comandos.
 */
void writeNumberOfFiles( int argc ){
    char cantArgStr[ MAX_BUFFER_SIZE ] = { 0 };
    sprintf( cantArgStr , "%d" , argc-1 );
    int aux = strlen( cantArgStr );
    cantArgStr[ aux ] = '\n';
    if ( write( STDOUT_FILENO , cantArgStr , aux + 1 ) == -1 ){
        handle_error( "write failed master" );
    }
}

/**
 * La función spawnSlaves se encarga de crear una cantidad de procesos hijos determinada por qSlaves y ordenar sus file descriptors para la
 * comunicación con el proceso padre.
 * @param qSlaves Cantidad de esclavos a crear.
 * @param slavesReadPipe Arreglo de FDs de lectura de pipes que conectan a los esclavos con el proceso padre.
 * @param slavesWritePipe Arreglo de FDs de escritura de pipes que conectan a los esclavos con el proceso padre.
 */
void spawnSlaves( int qSlaves , int slavesReadPipe[] , int slavesWritePipe[] ) {
    int k, l;

    for ( k = 0 ; k < qSlaves ; k++ ) {
        int forkStatus = fork();
        if ( forkStatus == -1 ) {
            handle_error( "fork failed master" );
        }

        /* Logica de cada hijo que pasa a ser esclavo */
        if ( forkStatus == 0 ) {
            /* Ordenamiento de FDs */
            if ( dup2( slavesReadPipe[ k ] , STDIN_FILENO ) == -1 ) {
                handle_error("dup master 1");
            }
            if ( dup2( slavesWritePipe[ k ] , STDOUT_FILENO ) == -1 ) {
                handle_error( "dup master 2" );
            }
            int totalFds = 4 * qSlaves + 4;
            for ( l = 3 ; l < totalFds ; l++ ) { /* cierro pipes sobrantes, y el fd de shm */
                close( l );
            }
            /* Transformacion a Esclavo */
            char * const paramList[] = { "slave.out" , NULL };
            execve( "slave.out" , paramList , NULL);
            handle_error( "execve" );
        }
    }
}

