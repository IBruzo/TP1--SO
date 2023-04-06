#include "m&s.h"

// El Master :
// Recibe los directorios de los archivos cuyos md5 se desea calcular ( no preocuparse por permisos o directorios invalidos )
// Inicia los procesos esclavos ( cantidad decidida arbitrariamente )
// Gestiona la cola de trabajo de los esclavos
// Recibe y almacena el resultado de cada esclavo en un buffer por orden de llegada
// Al ser inicializado debe esperar la aparición del proceso Vista, caso positivo debe compartir el buffer con dicho proceso
// Guarda el resultado en un archivos Results.txt

/*
LAST UPDATE :
- el slave puede comer digerir cualquier cantidad de archivos
- joaco usa este comando "make clean; rm results.txt; make all; ./master.out README.md master.c slave.c master.c master.c; make clean"
- por stdout devuelve info para debugear que use, y tambien crea el archivo results.txt

results.txt :
6af203164b9a2dc9dbbbbcf15edb0331  README.md
f4923e6253fdedc35888a5d022747d5a  master.c
f4923e6253fdedc35888a5d022747d5a  master.c
7f27e0c1a11d61bef5c11e893e37741a  slave.c
f4923e6253fdedc35888a5d022747d5a  master.c

*/

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
    int fdResults = open("./results.txt", O_APPEND|O_RDWR|O_CREAT , ALL_PERMISSIONS);
    // Declaro variables
    int i, j, N;                                                                        // variables de ciclos
    char* lineJump = "\n";                                                              // delimitador/separador
    // faltan los buffers aca ( backlog )
    // Calculo las subdivisiones
    int undigestedFiles = argc - 1;                                                     // archivos que faltan procesar
    int qSlaves = undigestedFiles/5;                                                    // cantidad de esclavos
    int initialLoad = undigestedFiles/10;                                               // factor de carga inicial
    int posNextFile = 1;                                                                // posicion del proximo archivo a procesar
    initialLoad=1;                      // HARDCODEADO INTENCIONAL
    // Declaro Arrays de esclavos
    int slavesReadPipe[qSlaves];                                                        // fd del cual cada esclavo lee el archivo que debe digerir
    int slavesWritePipe[qSlaves];                                                       // fd donde escribe el resultado de la digestion
    int masterReadPipe[qSlaves];                                                        // fd del cual el master lee el resultado de la digestion
    int masterWritePipe[qSlaves];                                                       // fd donde el master escribe el archivo a digerir
    char initiallyLoaded[qSlaves]; memset(initiallyLoaded, 0, sizeof(initiallyLoaded));
    char slaveStates[qSlaves];memset(slaveStates, 0, sizeof(slaveStates));              // cantidad de trabajo pendientes


    // TESTING
    printf("\n\t\tGeneral Info:\n");
    printf("\t\tUndigested Files   : %d\n", undigestedFiles);
    printf("\t\t#Slaves            : %d\n", qSlaves);
    printf("\t\tInitial Load       : %d\n", initialLoad);


    // Ordenamiento de pipes creados
    for (N = 0; N < qSlaves; N++ ){
        int mtosPipe[2];
        pipe(mtosPipe); CHECK_FAIL("pipe");
        slavesReadPipe[N] = mtosPipe[0];
        masterWritePipe[N] = mtosPipe[1];
        int stomPipe[2];
        pipe(stomPipe); CHECK_FAIL("pipe");
        masterReadPipe[N] = stomPipe[0];
        slavesWritePipe[N] = stomPipe[1];
    }


    // Creacion y Seteo del Select
    int nfds = 4*qSlaves+3;
    fd_set readfds;                                                                     // array con los fds a monitorear
    for ( N = 0; N < qSlaves; N++ )
        FD_SET(masterReadPipe[N], &readfds);                                            // seteo los fds de lectura del master


    /*
    Visualizacion de los File Descriptors :

    fd master = {       [ 0 - stdin ],      [ 1 - stdout ],         [ 2 - stderr ],        [ 3 - readpipe1 ],      [ 4 - writepipe1 ],
                        [ 5 - readpipe2 ],  [ 6 - writepipe2 ] .... [ 2N+1 - readpipeN ],  [ 2(N+1) - writepipeN ] }

    fd slave1 = { [ 3 - readpipe1 ],              [ 6 - writepipe2 ],         [ 2 - stderr ] }  (( OBJETIVO ))
    fd slave2 = { [ 7 - readpipe3 ],              [ 10 - writepipe4 ],        [ 2 - stderr ] }
    fd slave3 = { [ 11 - readpipe5 ],             [ 14 - writepipe6 ],        [ 2 - stderr ] }
    ...
    fd slaveN = { [ (4N-1) - readpipe(2N-1) ],    [ (4N+2) - writepipe(2N) ], [ 2 - stderr ] }
    */


    // Creacion de los Slaves
    printf("\n\t\tSlaves Creation:\n");
    for ( N = 0; N < qSlaves; N++ ){
        printf("\t\tCreation of Slave %d\n", N+1);
        int forkStatus = fork(); CHECK_FAIL("fork");
        if ( forkStatus == 0 ){                                                         // Logica de cada hijo :
            // Ordenamiento de FDs
            close(STDIN); dup(slavesReadPipe[N]); CHECK_FAIL("dup");                    // FD_0 =  FD_RD_END del mtosPipe
            close(STDOUT); dup(slavesWritePipe[N]); CHECK_FAIL("dup");                  // FD_1 =  FD_WR_END del stomPipe
            for( i = 3;  i < 4*(qSlaves)+3; i++ )                                       // cierro pipes sobrantes
                close(i);
            // Transformacion a Esclavo
            char * const paramList[] = {"slave.out", NULL};
            execve("slave.out", paramList, 0); CHECK_FAIL("execve");
        }
    }



    // Monitoreo y Escritura sobre results.txt de forma dinamica
    printf("\n\t\tDynamic Loading:\n");
    while( undigestedFiles != 0 ){

        // Se cargan los pipes con el Load Inicial
        for ( N = 0 ;N < qSlaves && posNextFile <= initialLoad*qSlaves; N++ ){
            if ( initiallyLoaded[N] == 0 && slaveStates[N]==0){
                for (j = posNextFile; j < (posNextFile+initialLoad); j++){
                    printf("\t\tStatic Loading of File '%s' to slave [ %d ]\n", argv[j], N+1);
                    char buffer[MAX_PATH_SIZE];
                    concat(argv[j], lineJump, buffer);
                    write(masterWritePipe[N], buffer, strlen(buffer)); CHECK_FAIL("write");
                    slaveStates[N]++;
                }
                posNextFile+=initialLoad;
                initiallyLoaded[N] = 1;
            }
        }

        char resultBuffer[MAX_PATH_SIZE];
        int bytesRead;
        // espero que haya una lectura no bloqueante de un pipe
        int qFdsToRead = select(nfds,&readfds, NULL, NULL, NULL);
        //printf("\t\tSelect activated with %d file descriptor(s) to read\n", qFdsToRead);
        for ( N = 0; N < qSlaves && qFdsToRead!=0; N++ ){ // HACER QUE SOLO REVISE LOS FDS QEU EL SELECT ESTA REVISANDO
            //printf("\n\t\tUndigested Files : %d    \n", undigestedFiles);
            //printf("\t\tPositionNextFile : %d    \n", posNextFile);
            //printf("\t\tChecking if master read pipe %d is readable, result = %d\n", N+1, FD_ISSET(masterReadPipe[N], &readfds));

            if ( FD_ISSET(masterReadPipe[N], &readfds)  ){
                bytesRead = read(masterReadPipe[N], resultBuffer, MAX_PATH_SIZE);CHECK_FAIL("read");
                //printf("\t\tbytes read : %d\n", bytesRead);
                //printf("Slave %d delivered %s\n", N+1, resultBuffer);
                write(fdResults, resultBuffer, bytesRead); CHECK_FAIL("write");

                slaveStates[N]--;
                if ( slaveStates[N] == 0 && undigestedFiles == 0 )
                    close(masterWritePipe[N]);

                qFdsToRead--;
                undigestedFiles--;

                if ( slaveStates[N] == 0 && posNextFile<argc ){
                    printf("\t\tDynamic Loading of File '%s' to slave [ %d ]\n", argv[posNextFile], N+1);            // Testing
                    char buffer[MAX_PATH_SIZE];
                    concat(argv[posNextFile], lineJump, buffer);
                    write(masterWritePipe[N],buffer, strlen(buffer)); CHECK_FAIL("write");
                    slaveStates[N]++;
                    posNextFile++;
                }
                //printf("\n\n");
                //for ( int j = 0; j < qSlaves; j++ )
                //    printf("\t unfinished jobs from slave [%d] : %d",j+1, slaveStates[j]);
                //printf("\n\n");

            }
        }
        FD_ZERO(&readfds);
        for ( N = 0; N < qSlaves; N++ ){
            if ( slaveStates[N] == 0){
                FD_SET(masterReadPipe[N], &readfds);
                FD_CLR(masterReadPipe[N], &readfds);
            }
            else
                FD_SET(masterReadPipe[N], &readfds);
        }
    }




    return 0;
}

/*
BACKLOG

- Comentar la seccion de carga
- Hay que pasar de strtok a getline
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

*/

