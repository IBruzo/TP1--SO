#include "m&s.h"

// El Master :
// Recibe los directorios de los archivos cuyos md5 se desea calcular ( no preocuparse por permisos o directorios invalidos )
// Inicia los procesos esclavos ( cantidad decidida arbitrariamente )
// Gestiona la cola de trabajo de los esclavos
// Recibe y almacena el resultado de cada esclavo en un buffer por orden de llegada
// Al ser inicializado debe esperar la aparición del proceso Vista, caso positivo debe compartir el buffer con dicho proceso
// Guarda el resultado en un archivos Results.txt

/**
@param str1 string que funciona de base
@param str2 string que se pone atras del str1
@param buffer resultado de la concatenacion
*/
void concat(const char* str1, const char* str2, char* buffer);
void loadFileName(char * files[],int posFiles, int * whereToWrite, int posFd);

int main(int argc, char *argv[])
{
    // Seteo en 0 de variable global para manejo de errores
    errno = 0;
    // Creacion de archivo de resultados
    int fdResults = open("./results.txt", O_APPEND | O_RDWR | O_CREAT , ALL_PERMISSIONS);
    // Declaro variables
    int i, j, n;                                                                        // variables de ciclos
    // Calculo las subdivisiones
    int undigestedFiles = argc - 1;                                                     // archivos que faltan procesar
    int qSlaves = (int) ceil(undigestedFiles/5.0);                                      // cantidad de esclavos
    int posNextFile = 1;                                                                // posicion del proximo archivo a procesar
    int initialLoad = (int) ceil(undigestedFiles/10.0);                                 // factor de carga inicial
    // Declaro Arrays de esclavos
    int slavesReadPipe[qSlaves];                                                        // fd del cual cada esclavo lee el archivo que debe digerir
    int slavesWritePipe[qSlaves];                                                       // fd donde escribe el resultado de la digestion
    int masterReadPipe[qSlaves];                                                        // fd del cual el master lee el resultado de la digestion
    int masterWritePipe[qSlaves];                                                       // fd donde el master escribe el archivo a digerir
    char initiallyLoaded[qSlaves];                                                      // boolean de si fue cargada o no
    memset(initiallyLoaded, 0, sizeof(initiallyLoaded));
    char slaveStates[qSlaves];                                                          // cantidad de trabajo pendientes
    memset(slaveStates, 0, sizeof(slaveStates));


    // Ordenamiento de pipes creados
    for (n = 0; n < qSlaves; n++){
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
    int shm_fd;                                                                         // fd de shared memory
    char * shm_ptr;                                                                     // puntero a la shared memory
    sem_t * sem;                                                                        // counting semaphore

    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);                                         // se crea/obtiene el fd del semaforo
    CHECK_FAIL("sem_open");

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);                                // fd de la shared memory
    CHECK_FAIL("shm_fd");

    ftruncate(shm_fd, SHM_SIZE);                                                        // se ajusta el tamaño deseado a la shm
    CHECK_FAIL("ftruncate");

    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);      // mapeo a la memoria virtual de este proceso
    CHECK_FAIL("mmap");

    sleep(5);                                                                           // Se espera a la ejecucion del proceso vista

    // Retorno de la informacion requerida por el Vista por STDOUT (cantidad de archivos a hashear)
    char cantArgStr[MAX_BUFFER_SIZE] = {0};
    sprintf(cantArgStr,"%d",argc-1);
    int aux = strlen(cantArgStr);
    cantArgStr[aux]='\n';
    write(STDOUT,cantArgStr, aux+1);
    CHECK_FAIL("write");


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
        int forkStatus = fork(); CHECK_FAIL("fork");
        // Logica de cada hijo que pasa a ser esclavo
        if (forkStatus == 0){
            // Ordenamiento de FDs
            close(STDIN); dup(slavesReadPipe[n]);
            CHECK_FAIL("dup");                                                          // FD_0 =  FD_RD_END del mtosPipe
            close(STDOUT); dup(slavesWritePipe[n]);
            CHECK_FAIL("dup");                                                          // FD_1 =  FD_WR_END del stomPipe
            for(i = 3;  i < 4*(qSlaves)+4; i++)                                       // cierro pipes sobrantes, y el fd de shm
                close(i);
            // Transformacion a Esclavo
            char * const paramList[] = {"slave.out", NULL};
            execve("slave.out", paramList, 0);
            CHECK_FAIL("execve");
        }
    }



    // Monitoreo y Escritura sobre results.txt de forma dinamica
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
        int qFdsToRead = select(nfds,&readfds, NULL, NULL, NULL);
        for (n = 0; n < qSlaves && qFdsToRead != 0 ; n++){
            if (FD_ISSET(masterReadPipe[n] , &readfds)){
                // lectura del buffer del slave que escribio su resultado
                char resultBuffer[MAX_BUFFER_SIZE];
                int bytesRead;
                bytesRead = read(masterReadPipe[n], resultBuffer, MAX_BUFFER_SIZE);
                CHECK_FAIL("read");

                // escritura en el archivo Results.txt
                write(fdResults, resultBuffer, bytesRead);
                CHECK_FAIL("write");
                // escritura en la memoria compartida
                strcpy(shm_ptr,resultBuffer); // mandamos la shared memory
                sem_post(sem); //levantamos el semaforo asi no se bloquea

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
    for (i = 3; i < 4 * qSlaves + 3; i++)
        close(i);
    sem_close(sem);
    shm_unlink(SHM_NAME);
    return 0;
}

// concatena dos strings pero a diferencia de strcat no altera el string dest
void concat(const char* str1, const char* str2, char* buffer) {
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    size_t size = len1 + len2 + 1;
    strncpy(buffer, str1, len1);
    strncpy(buffer + len1, str2, len2);
    buffer[size - 1] = '\0';
}

// carga el pipe compartido entre master y slave con el file requerido
void loadFileName(char * files[],int posFiles, int * whereToWrite, int posFd){
    char buffer[MAX_PATH_SIZE];
    concat(files[posFiles], "\n", buffer);
    write(whereToWrite[posFd],buffer, strlen(buffer));
    CHECK_FAIL("write");
}

