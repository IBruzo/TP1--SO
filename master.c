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

void transferPipeValues( int* pipeValues, int destination[][2], int position){
    destination[position][0] = pipeValues[0];
    destination[position][1] = pipeValues[1];
    return;
}

int main(int argc, char *argv[])
{


    errno = 0;                                                          // seteo en 0 para manejo de errores


    // Creacion de archivo de resultados
    int fdResults = open("./results.txt", O_APPEND|O_RDWR|O_CREAT , ALL_PERMISSIONS);

    // Variables de ciclos
    int posNextFile = 0;
    int i, j, N;

    // Calculo las subdivisiones
    int undigestedFiles = argc - 1;
    int qSlaves = undigestedFiles/5;
    int initialLoad = undigestedFiles/10;


    // Array de cantidad de trabajos
    char slaveStates[qSlaves];memset(slaveStates, 0, sizeof(slaveStates));              // cantidad de trabajo pendientes
    // Arrays de FDs, dividos por funcion
    int slavesReadPipe[qSlaves];                                                        // fd del cual cada esclavo lee el archivo que debe digerir
    int slavesWritePipe[qSlaves];                                                       // fd donde escribe el resultado de la digestion
    int masterReadPipe[qSlaves];                                                        // fd del cual el master lee el resultado de la digestion
    int masterWritePipe[qSlaves];                                                       // fd donde el master escribe el archivo a digerir


    // TESTING
    printf("\n\t\tGeneral Info:\n");
    printf("\t\tUndigested Files   : %d\n", undigestedFiles);
    printf("\t\t#Slaves            : %d\n", qSlaves);
    printf("\t\tInitial Load       : %d\n", initialLoad);


    // Ordenamiento de los pipes creados
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


    // Creacion y seteo de FDs necesarios para el select
    int nfds = 4*qSlaves+3;
    fd_set readfds;
    FD_ZERO(&readfds);
    for ( N = 0; N < qSlaves; N++ )
        FD_SET(masterReadPipe[N], &readfds);                                            // monitoreo si el extremo de lectura del master es bloqueante o no



    // Carga inicial de los pipes
    printf("\n\t\tInitial Loading:\n");
    char* lineJump = "\n";                                                              // delimitador/separador
    for ( posNextFile = 1; posNextFile <= initialLoad*qSlaves; posNextFile++){          // recorro los archivos entrantes
        printf("\t\tLoading file %s to slave --- %d ---\n", argv[posNextFile], (posNextFile-1)%qSlaves+1);
        char buffer[MAX_PATH_SIZE];
        concat(argv[posNextFile], lineJump, buffer);
        write(masterWritePipe[(posNextFile-1)%qSlaves], buffer, strlen(buffer)); CHECK_FAIL("write");
        slaveStates[(posNextFile-1)%qSlaves]++;
    }

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
    int childStatus;
    for ( N = 0; N < qSlaves; N++ ){
        printf("\t\tCreation of Slave %d\n", N+1);
        int forkStatus = fork(); CHECK_FAIL("fork");
        if ( forkStatus == 0 ){                                                         // Cada esclavo
            // Ordenamiento de FDs
            close(STDIN); dup(slavesReadPipe[N]); CHECK_FAIL("dup");                    // muevo al fd 0 su FD de lectura
            close(STDOUT); dup(slavesWritePipe[N]); CHECK_FAIL("dup");                  // muevo al fd 1 su FD de escritura
            for( i = 3;  i < 4*(qSlaves)+3; i++ )                                       // cierro pipes sobrantes
                close(i);
            // creacion del slave N
            char * const paramList[] = {"slave.out", NULL};
            execve("slave.out", paramList, 0); CHECK_FAIL("execve");
        }
    }


    // TESTING
    printf("\n\t\tGeneral Info:\n");
    printf("\t\tUndigested Files   : %d\n", undigestedFiles);
    printf("\t\t#Slaves            : %d\n", qSlaves);
    printf("\t\tInitial Load       : %d\n", initialLoad);


    // Monitoreo y Escritura sobre results.txt de forma dinamica
    printf("\n\t\tDynamic Loading:\n");
    while( undigestedFiles != 0 ){
        char resultBuffer[MAX_PATH_SIZE];
        int bytesRead;
        int qFdsToRead = select(nfds,&readfds, NULL, NULL, NULL);
        //printf("\t\tSelect activated with %d file descriptor(s) to read\n", qFdsToRead);
        for ( N = 0; N < qSlaves && qFdsToRead!=0; N++ ){ // HACER QUE SOLO REVISE LOS FDS QEU EL SELECT ESTA REVISANDO
            //printf("\n\t\tUndigested Files : %d    \n", undigestedFiles);


            //printf("\n\t\tPositionNextFile : %d    \n", posNextFile);
            //printf("\t\tChecking if master read pipe %d is readable, result = %d\n", N+1, FD_ISSET(masterReadPipe[N], &readfds));

            if ( FD_ISSET(masterReadPipe[N], &readfds)  ){

                // int retSize =  MD5_SIZE+strlen(argv[posNextFile])+3;                              // md5sum ret = ( codificacion + "  " + dir + "\n" )
                bytesRead = read(masterReadPipe[N], resultBuffer, MAX_PATH_SIZE);CHECK_FAIL("read");     // lee de [ 5 - readpipe2 ]
                //printf("\t\tbytes read : %d\n", bytesRead);                             // TESTING
                printf("Slave %d delivered %s\n", N+1, resultBuffer);
                write(fdResults, resultBuffer, bytesRead); CHECK_FAIL("write");

                slaveStates[N]--;
                if ( slaveStates[N] == 0 && undigestedFiles == 0 ){
                    close(masterWritePipe[N]);
                }
                qFdsToRead--;
                undigestedFiles--;

                if ( slaveStates[N] == 0 && posNextFile<argc ){
                    printf("\t\tLoading file %s to slave --- %d ---\n", argv[posNextFile], N+1);            // Testing
                    char buffer[MAX_PATH_SIZE];
                    concat(argv[posNextFile], lineJump, buffer);
                    write(masterWritePipe[N],buffer, strlen(buffer)); CHECK_FAIL("write");
                    slaveStates[N]++;
                    posNextFile++;                                         // trabajando
                }
                printf("\n\n");
                for ( int j = 0; j < qSlaves; j++ )
                    printf("\t unfinished jobs from slave [%d] : %d",j+1, slaveStates[j]);
                printf("\n\n");

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
Funcionamiento ejemplificado de master y slave

- se ejecuta el comando en bash con 100 archivos
- el master recibe los archivos, 100 archivos
- decide cuantos esclavos quiere usar, 5 esclavos
- cada esclavo esclavo recibe la cantidad de archivos que puede digerir inicialmente, 5 archivos
- a medida que cada esclavo termina su tarea el master le vuelve a introducir UN archivo

*/

/*
Posibles improvements y tips :
- lo mas practico es que cada slave tome un solo archivo para maximizar el paralelismo, despues de la carga inicial
- ( ! ) el master es el unico que sabe de la existencia del archivo
- hay que closear todos los fd abiertos al final, cerrar el w-end del pipe envia un eof y se termina el while de lectura
- combinar la idea de shared memory y results.txt, esta en la clase del 3/4
- toda comunicacion entre master y slave debe ser a traves del pipe ( lo menciono en referencia al pasaje del factor inicial de los slaves, entonces no es fijo ( ??? ) )
- envias tareas al slave cada vez este ocioso ( ocioso es un concepto del master, si le envio dos y recibo dos resultados es porque ya esta ocioso )
- hay que tener cuidado con el read de los pipes del master, se usa un select o select_tut que monitorea las activaciones de distintas fds, explicado al finald e la clase del 3/4
- sobre el select : el select se acciona, loopeas en los fds, encontras el que hizo saltar el select ( ISSET  macro ) y procedes a hacer el read al que da TRUE ( no hacer dos seguidos y puede retornar el eof ), reinicializar los fds en cada loop
- hay cosas hechas con syscalls que podrian hacerse con libreria de c, en especial manejo de archivos
. modularizacion de funciones, loadPipe para cuando haya muchos slaves

- REFACTORIZAR A 4 ARRAYS
SLAVEREADPIPE[N]
SLAVEWRITEPIPE[N]
MASTERREADPIPE[N]
MASTERWRITEPIPE[N]
*/

