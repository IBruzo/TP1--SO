#include "masterAndSlave.h"
/* 
 * El Master :
 * Recibe los directorios de los archivos cuyos md5 se desea calcular ( no preocuparse por permisos o directorios invalidos )
 * Inicia los procesos esclavos ( cantidad decidida arbitrariamente )
 * Gestiona la cola de trabajo de los esclavos
 * Recibe y almacena el resultado de cada esclavo en un buffer por orden de llegada
 * Al ser inicializado debe esperar la aparición del proceso Vista, caso positivo debe compartir el buffer con dicho proceso
 * Guarda el resultado en un archivos Results.txt
 */
void concat( const char* str1 , const char* str2 , char* buffer );
void loadFileName( char * files[] ,int posFiles , int * whereToWrite , int posFd );

int main( int argc , char *argv[] )
{
    // Seteo en 0 de variable global para manejo de errores
    errno = 0;
    // Creacion de archivo de resultados
    int fdResults = open( "./results.txt", O_APPEND | O_RDWR | O_CREAT , ALL_PERMISSIONS );
    // Declaro variables
    int i, j, n;                                                                        // variables de ciclos
    // Calculo las subdivisiones
    int undigestedFiles = argc - 1;                                                     // archivos que faltan procesar
    int qSlaves = (int) ceil(undigestedFiles/5.0);                                      // cantidad de esclavos
    int posNextFile = 1;                                                                // posicion del proximo archivo a procesar
    int initialLoad = (int) ceil(undigestedFiles/10.0);                                 // factor de carga inicial
    // Declaro Arrays de esclavos
    int slavesReadPipe[ qSlaves ];                                                        // fd del cual cada esclavo lee el archivo que debe digerir
    int slavesWritePipe[ qSlaves ];                                                       // fd donde escribe el resultado de la digestion
    int masterReadPipe[ qSlaves ];                                                        // fd del cual el master lee el resultado de la digestion
    int masterWritePipe[ qSlaves ];                                                       // fd donde el master escribe el archivo a digerir
    char initiallyLoaded[ qSlaves ];                                                      // boolean de si fue cargada o no
    memset( initiallyLoaded , 0 , sizeof( initiallyLoaded ) );
    char slaveStates[ qSlaves ];                                                          // cantidad de trabajo pendientes
    memset( slaveStates , 0 , sizeof( slaveStates ) );


    // Ordenamiento de pipes creados
    for ( n = 0 ; n < qSlaves ; n++){
        int mtosPipe[ 2 ];
        if( pipe( mtosPipe ) < 0 ){
            handle_error("pipe");
        } 
        slavesReadPipe[n] = mtosPipe[0];
        masterWritePipe[n] = mtosPipe[1];

        int stomPipe[2];
        if( pipe( stomPipe ) < 0 ){
            handle_error("pipe");
        } 
        masterReadPipe[n] = stomPipe[0];
        slavesWritePipe[n] = stomPipe[1];
    }


    // Manejo de Semaforos y Shared Memory
    int shm_fd;                                                                         // fd de shared memory
    char * shm_ptr;                                                                     // puntero a la shared memory
    sem_t * sem;                                                                        // counting semaphore

    sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);                                         // se crea/obtiene el fd del semaforo
    if (sem == SEM_FAILED) {
        handle_error("sem_open failed");                                                       // prints the error message to stderr
    }

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);                                // fd de la shared memory
    if(shm_fd == -1){
        handle_error("shm_open failed master");
    }

    if( ftruncate(shm_fd, SHM_SIZE) == -1 ){                                             // se ajusta el tamaño deseado a la shm
        handle_error("ftrncate failed");
    }  
        
    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);      // mapeo a la memoria virtual de este proceso
    if (shm_ptr == MAP_FAILED) {
        handle_error("mmap failed");
    }

    sleep(5);                                                                           // Se espera a la ejecucion del proceso vista

    // Retorno de la informacion requerida por el Vista por STDOUT (cantidad de archivos a hashear)
    char cantArgStr[MAX_BUFFER_SIZE] = {0};
    sprintf(cantArgStr,"%d",argc-1);
    int aux = strlen(cantArgStr);
    cantArgStr[aux]='\n';
    if(write(STDOUT_FILENO,cantArgStr, aux+1)==-1){
        handle_error("write failed");
    }


    // Creacion y Seteo del Select
    int nfds = 4 * qSlaves + 3;
    fd_set readfds;                                                                     // array con los fds a monitorear
    FD_ZERO(&readfds);                                                                  // limpio el set
    for (n = 0; n < qSlaves; n++){
        FD_SET(masterReadPipe[n], &readfds);                                            // seteo los fds de lectura del master
    }


    /*
    Visualizacion de los File Descriptors :

    fd master = {       [ 0 - stdin ],      [ 1 - stdout ],         [ 2 - stderr ],        [ 3 - readpipe1 ],      [ 4 - writepipe1 ],
                        [ 5 - readpipe2 ],  [ 6 - writepipe2 ] .... [ 2N+1 - readpipeN ],  [ 2(N+1) - writepipeN ], [ 2(N+1)+1 - shm ] }

    fd slave1 = { [ 3 - readpipe1 ],              [ 6 - writepipe2 ],         [ 2 - stderr ] }  (( OBJETIVO ))
    fd slave2 = { [ 7 - readpipe3 ],              [ 10 - writepipe4 ],        [ 2 - stderr ] }
    fd slave3 = { [ 11 - readpipe5 ],             [ 14 - writepipe6 ],        [ 2 - stderr ] }
    ...
    fd slaveN = { [ (4N-1) - readpipe(2N-1) ],    [ (4N+2) - writepipe(2N) ], [ 2 - stderr ] }
    */


    // Creacion de los Slaves
    for (n = 0; n < qSlaves; n++){
        int forkStatus = fork();
        if(forkStatus == -1){
            handle_error("fork failed");
        }
        
        // Logica de cada hijo que pasa a ser esclavo
        if (forkStatus == 0){
            // Ordenamiento de FDs
            if (dup2(slavesReadPipe[n], STDIN_FILENO) == -1){
                handle_error("dup");
            }
            if (dup2(slavesWritePipe[n], STDOUT_FILENO) == -1){
                handle_error("dup");
            }
            for(i = 3;  i < 4*(qSlaves)+4; i++){
                close(i);
            }                                       // cierro pipes sobrantes, y el fd de shm
            // Transformacion a Esclavo
            char * const paramList[] = {"slave.out", NULL};
            execve("slave.out", paramList, NULL);
            handle_error("execve");
        }
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    // Monitoreo y Escritura sobre results.txt de forma dinamica
    int offset=0;
    while(undigestedFiles != 0){
        // Cargado inicial de los buffers personalizados para cada slave con la carga inicial
        for (n = 0 ; n < qSlaves && posNextFile <= initialLoad * qSlaves ; n++){
            if ( initiallyLoaded[n] == 0 && slaveStates[n] == 0 ){
                for (j = posNextFile ; j < (posNextFile + initialLoad) ; j++){
                    loadFileName(argv,j,masterWritePipe,n);
                    slaveStates[n]++;
                }
                posNextFile += initialLoad;
                initiallyLoaded[n] = 1;
            }
        }

        // Monitoreo de Fds para obtener una lectura no bloqueante
        int qFdsToRead = select(nfds, &readfds, NULL, NULL, &tv);
        
        for (n = 0; n < qSlaves && qFdsToRead != 0 ; n++){
            if (FD_ISSET(masterReadPipe[n] , &readfds)){
                // lectura del buffer del slave que escribio su resultado
                char resultBuffer[MAX_BUFFER_SIZE];
                int bytesRead;
                bytesRead = read(masterReadPipe[n], resultBuffer, MAX_BUFFER_SIZE);
                if (bytesRead == -1) {
                    handle_error("read failed");
                }

                // escritura en el archivo Results.txt
                int bytesWritten = write(fdResults, resultBuffer, bytesRead);
                if(bytesWritten == -1){
                    handle_error("write failed");
                }
                
                
                    
                // escritura en la memoria compartida
                strcpy(shm_ptr + offset, resultBuffer); // mandamos la shared 
                offset += strlen(resultBuffer) ;

                if( sem_post(sem) == - 1 ){//levantamos el semaforo asi no se bloquea
                    handle_error("sem_post failed");
                } 
                int value;
                if (sem_getvalue(sem, &value) == -1) {
                    perror("sem_getvalue");
                    exit(EXIT_FAILURE);
                }

                // movimiento de variables
                slaveStates[n]--;
                qFdsToRead--;
                undigestedFiles--;

                // caso donde un slave consumio los archivos que se le dieron, entonces se le carga un archivo nuevo
                if (slaveStates[n] == 0 && posNextFile < argc){
                    loadFileName(argv, posNextFile, masterWritePipe, n);
                    slaveStates[n]++;
                    posNextFile++;
                }
            }
        }
        // Reiniciado del Select, si un slave ya no puede trabajar porque no hay mas archivos se retira su fd del select
        FD_ZERO(&readfds);
        for (n = 0; n < qSlaves; n++){
            if (slaveStates[n] == 0){
                FD_SET(masterReadPipe[n], &readfds);
                FD_CLR(masterReadPipe[n], &readfds);
            }
            else
                FD_SET(masterReadPipe[n], &readfds);
        }
    }


    //Cerrado de Fd del master, shared memory y semaforos
    for (i = 3; i < 4 * qSlaves + 3; i++){
        if(close(i) == - 1){
            handle_error("close failed");
        }
    }
    // cerrar semaforo
    if (sem_close(sem) == -1) {
        handle_error("sem_close failed");
    }

    // Unlink shared memory
    if (munmap(shm_ptr, SHM_SIZE) == -1) {
        handle_error("munmap failed");
    }
    if (shm_unlink(SHM_NAME) == -1) {
        handle_error("shm_unlink failed");
    }
    if (close(shm_fd) == -1) {
        handle_error("close failed");
    }
    // Unlink semaforo
    if (sem_unlink(SEM_NAME) == -1) {
        handle_error("sem_unlink failed");
    }


    return 0;
}





/**
 * Función que concatena dos cadenas de caracteres en un buffer, sin modificar la cadena original de destino.
 * @param str1 La primera cadena de caracteres a concatenar.
 * @param str2 La segunda cadena de caracteres a concatenar.
 * @param buffer Puntero al buffer donde se almacenará la cadena resultante de la concatenación.
*/
void concat(const char* str1, const char* str2, char* buffer) {
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    size_t size = len1 + len2 + 1;
    strncpy(buffer, str1, len1);
    strncpy(buffer + len1, str2, len2);
    buffer[size - 1] = '\0';
}

/**
 * Carga en el pipe compartido entre master y slave el nombre del archivo requerido.
 * @param files Arreglo de strings con los nombres de los archivos.
 * @param posFiles Posición en el arreglo de strings del archivo a cargar.
 * @param whereToWrite Arreglo de file descriptors para escribir en los pipes.
 * @param posFd Posición en el arreglo de file descriptors del pipe a utilizar.
 */
void loadFileName(char * files[],int posFiles, int * whereToWrite, int posFd){
    char buffer[MAX_PATH_SIZE];
    concat(files[posFiles], "\n", buffer);
    if(write(whereToWrite[posFd],buffer, strlen(buffer))==-1){
        handle_error("write failed");
    }

}

