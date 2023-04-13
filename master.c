#include "m&s.h"

// El Master :
// Recibe los directorios de los archivos cuyos md5 se desea calcular ( no preocuparse por permisos o directorios invalidos )
// Inicia los procesos esclavos ( cantidad decidida arbitrariamente )
// Gestiona la cola de trabajo de los esclavos
// Recibe y almacena el resultado de cada esclavo en un buffer por orden de llegada
// Al ser inicializado debe esperar la aparición del proceso Vista, caso positivo debe compartir el buffer con dicho proceso
// Guarda el resultado en un archivos Results.txt

// PASAR A UNA LIB
void concat(const char* str1, const char* str2, char* buffer) {
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    size_t size = len1 + len2 + 1;
    strncpy(buffer, str1, len1);
    strncpy(buffer + len1, str2, len2);
    buffer[size - 1] = '\0';
}

int main(int argc, char *argv[])
{
    // Seteo en 0 de variable global para manejo de errores
    errno = 0;
    // Creacion de archivo de resultados
    int fdResults = open("./results.txt", O_APPEND | O_RDWR | O_CREAT , ALL_PERMISSIONS);
    // Declaro variables
    int i, j, n;                                                                        // variables de ciclos
    char * lineJump = "\n";                                                              // delimitador/separador
    // faltan los buffers aca ( backlog )
    // Calculo las subdivisiones
    int undigestedFiles = argc - 1;                                                     // archivos que faltan procesar
    int qSlaves = (int) ceil(undigestedFiles/5.0);                                                    // cantidad de esclavos
    int posNextFile = 1;                                                                // posicion del proximo archivo a procesar
    int initialLoad = (int) ceil(undigestedFiles/10.0);                                               // factor de carga inicial
    // Declaro Arrays de esclavos
    int slavesReadPipe[qSlaves];                                                        // fd del cual cada esclavo lee el archivo que debe digerir
    int slavesWritePipe[qSlaves];                                                       // fd donde escribe el resultado de la digestion
    int masterReadPipe[qSlaves];                                                        // fd del cual el master lee el resultado de la digestion
    int masterWritePipe[qSlaves];                                                       // fd donde el master escribe el archivo a digerir
    char initiallyLoaded[qSlaves]; 
    memset(initiallyLoaded, 0, sizeof(initiallyLoaded));
    char slaveStates[qSlaves];
    memset(slaveStates, 0, sizeof(slaveStates));              // cantidad de trabajo pendientes


    // TESTING
    ////printf("\n\t\tGeneral Info:\n");
    ////printf("\t\tUndigested Files   : %d\n", undigestedFiles);
    ////printf("\t\t#Slaves            : %d\n", qSlaves);
    ////printf("\t\tInitial Load       : %d\n", initialLoad);


    // Ordenamiento de pipes creados
    for (n = 0; n < qSlaves; n++ ){
        int mtosPipe[2];
        pipe(mtosPipe); CHECK_FAIL("pipe");
        slavesReadPipe[n] = mtosPipe[0];
        masterWritePipe[n] = mtosPipe[1];
        
        int stomPipe[2];
        pipe(stomPipe); CHECK_FAIL("pipe");
        masterReadPipe[n] = stomPipe[0];
        slavesWritePipe[n] = stomPipe[1];
    }


    // Manejo de Semaforos y Shared Memory
    int shm_fd;             // Aquí guardaremos el file descriptor de la shared memory
    char * shm_ptr;          // puntero de escritura en la shared memory
    sem_t * sem;            // counting semaphore

    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1); // creacion del semaforo, chequear inicializacion en 1
    CHECK_FAIL("sem_open");         

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);// creación de la shared memory
    CHECK_FAIL("shm_fd");

    ftruncate(shm_fd, SHM_SIZE); // Darle el tamaño deseado a la shm
    CHECK_FAIL("ftruncate");

    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); // mapeo a la memoria virtual
    CHECK_FAIL("mmap");   
      // Se espera a la ejecucion del proceso vista
    sleep(5);
  

    char cantArgStr[MAX_BUFFER_SIZE] = {0};
    sprintf(cantArgStr,"%d",argc-1);
    int aux = strlen(cantArgStr);
    cantArgStr[aux]='\n';
    write(STDOUT,cantArgStr, aux+1);         // en caso de uso con pipes
    

    // Creacion y Seteo del Select
    int nfds = 4 * qSlaves + 3;
    fd_set readfds;                                                                     // array con los fds a monitorear
    FD_ZERO(&readfds);                                                                  // limpio el set
    for ( n = 0; n < qSlaves; n++ ){
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
    ////printf("\n\t\tSlaves Creation:\n");
    for ( n = 0; n < qSlaves; n++ ){
        ////printf("\t\tCreation of Slave %d\n", n+1);
        int forkStatus = fork(); CHECK_FAIL("fork");
        if ( forkStatus == 0 ){                                                         // Logica de cada hijo :
            // Ordenamiento de FDs
            close(STDIN); dup(slavesReadPipe[n]); 
            CHECK_FAIL("dup");                    // FD_0 =  FD_RD_END del mtosPipe
            close(STDOUT); dup(slavesWritePipe[n]); 
            CHECK_FAIL("dup");                  // FD_1 =  FD_WR_END del stomPipe
            for( i = 3;  i < 4*(qSlaves)+4; i++ )                                       // cierro pipes sobrantes, y el fd de shm
                close(i);
            // Transformacion a Esclavo
            char * const paramList[] = {"slave.out", NULL};
            execve("slave.out", paramList, 0); 
            CHECK_FAIL("execve");
        }
    }



    // Monitoreo y Escritura sobre results.txt de forma dinamica
    // printf("\n\t\tStatic & Dynamic Loading:\n");
    while( undigestedFiles != 0 ){

        // Se cargan los pipes con el Load Inicial, cargado estatico
        for ( n = 0 ; n < qSlaves && posNextFile <= initialLoad * qSlaves ; n++ ){
            if ( initiallyLoaded[n] == 0 && slaveStates[n] == 0 ){
                for ( j = posNextFile ; j < (posNextFile + initialLoad) ; j++ ){
                    ////printf("\t\tStatic Loading of File '%s' to slave [ %d ]\n", argv[j], n+1);
                    char buffer[MAX_PATH_SIZE];
                    concat(argv[j] , lineJump , buffer);
                    write(masterWritePipe[n] , buffer , strlen(buffer)); CHECK_FAIL("write");
                    slaveStates[n]++;
                }
                posNextFile+=initialLoad;
                initiallyLoaded[n] = 1;
            }
        }

        char resultBuffer[MAX_BUFFER_SIZE];
        int bytesRead;
        // espero que haya una lectura no bloqueante de un pipe
        int qFdsToRead = select(nfds,&readfds, NULL, NULL, NULL);
        //printf("\t\tSelect activated with %d file descriptor(s) to read\n", qFdsToRead);
        for ( n = 0; n < qSlaves && qFdsToRead != 0 ; n++ ){
            //printf("\n\t\tUndigested Files : %d    \n", undigestedFiles);
            //printf("\t\tPositionNextFile : %d    \n", posNextFile);
            //printf("\t\tChecking if master read pipe %d is readable, result = %d\n", N+1, FD_ISSET(masterReadPipe[N], &readfds));

            if ( FD_ISSET(masterReadPipe[n] , &readfds)  ){
                bytesRead = read(masterReadPipe[n], resultBuffer, MAX_BUFFER_SIZE); CHECK_FAIL("read");
                //printf("\t\tbytes read : %d\n", bytesRead);
                //printf("Slave %d delivered %s\n", N+1, resultBuffer);
                // escritura en el archivo Results.txt
                write(fdResults, resultBuffer, bytesRead); CHECK_FAIL("write");
                // escritura en la memoria compartida
             //   sem_wait(sem);                // comienzo de la region critica
            //write(shm_fd, resultBuffer, bytesRead); CHECK_FAIL("write");

                strcpy(shm_ptr,resultBuffer);
                sem_post(sem);                // fin de la region critica
                // up(&canRead);

                slaveStates[n]--;
                qFdsToRead--;
                undigestedFiles--;

                if ( slaveStates[n] == 0 && posNextFile<argc ){
                    //// printf("\t\tDynamic Loading of File '%s' to slave [ %d ]\n", argv[posNextFile], n+1);            // Testing
                    char buffer[MAX_PATH_SIZE];
                    concat(argv[posNextFile], lineJump, buffer);
                    write(masterWritePipe[n],buffer, strlen(buffer)); CHECK_FAIL("write");
                    slaveStates[n]++;
                    posNextFile++;
                }
                //printf("\n\n");
                //for ( int j = 0; j < qSlaves; j++ )
                //    printf("\t unfinished jobs from slave [%d] : %d\n",j+1, slaveStates[j]);
                //printf("\n\n");

            }
        }
        FD_ZERO(&readfds);
        for ( n = 0; n < qSlaves; n++ ){
            if ( slaveStates[n] == 0){
                FD_SET(masterReadPipe[n], &readfds);
                FD_CLR(masterReadPipe[n], &readfds);
            }
            else
                FD_SET(masterReadPipe[n], &readfds);
        }
    }
    for ( i = 3; i < 4*qSlaves+4; i++ )
        close(i);
    sem_close(sem);
    shm_unlink(SHM_NAME);
    return 0;
}

/*
BACKLOG

- Comentar la seccion de carga
- Hacer todo lo referido a Vista
- Renombrar los buffers y ponerles tamaños adecuados, actualmente todos crean uno y le poenne MAXPATHSIZE

*/

/*
Posibles improvements y tips :

- De alguna forma integrar el load estatico al load dinamico <- PREGUNTAR A ARIEL
- Combinar la idea de shared memory y results.txt, esta en la clase del 3/4
- Toda comunicacion entre master y slave debe ser a traves del pipe ( lo menciono en referencia al pasaje del factor inicial de los slaves, entonces no es fijo ( ??? ) )
- Hay cosas hechas con syscalls que podrian hacerse con libreria de c, en especial manejo de archivos, tal vez es mejor usar syscalls?
. modularizacion de funciones!
- tamaño dinamico de la shared memory

*/
