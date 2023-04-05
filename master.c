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

void transferPipeValues( int* pipeValues, int destination[][2], int position){
    destination[position][0] = pipeValues[0];
    destination[position][1] = pipeValues[1];
    return;
}

int main(int argc, char *argv[])
{


    errno = 0;                                                          // seteo en 0 para manejo de errores
    // variables de ciclos
    int i, N;

    // calculo las subdivisiones
    int undigestedFiles = argc - 1;
    int qSlaves = undigestedFiles/5;                                               // ej : recibo 20 archivos -> se usaran 4 slaves y
    int initialLoad = undigestedFiles/10;                                          //      c/u recibe 2 de archivos como factor de carga inicial
    // array con los fd del master
    int slavePipes[2*qSlaves][2];                                       // 2 pipes por cada slave, cada pipe tiene read y write,
    // array con los estados de cada slave
    //char slaveState[qSlaves];                                           // estado de los slaves, ocioso o trabajando

    // TESTING
    printf("\t\tArguments received : %d\n", undigestedFiles);
    printf("\t\t#Slaves            : %d\n", qSlaves);
    printf("\t\tInitial Load       : %d\n", initialLoad);

    // creacion de pipes, tiene que haber una manera mas elegante @chatgpt
    for (i = 0; i < qSlaves; i++ ){
        int mtosPipe[2];
        int stomPipe[2];
        pipe(mtosPipe); CHECK_FAIL("pipe");
        transferPipeValues(mtosPipe, slavePipes,2*i);
        pipe(stomPipe); CHECK_FAIL("pipe");
        transferPipeValues(stomPipe, slavePipes,2*i+1);
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



    // creacion de esclavos con sus respectivos pipes de comunicacion
    int childStatus;
    for ( N = 0; N < qSlaves; N=N+2 ){
        printf("\t\tCreacion de esclavo %d\n", N+1);
        int forkStatus = fork(); CHECK_FAIL("fork");
        if ( forkStatus == 0 ){                                                         // Cada esclavo
            close(STDIN); dup(slavePipes[N][RD_END]); CHECK_FAIL("dup");                // muevo al fd 0 el read del pipe 4N-1
            close(STDOUT); dup(slavePipes[N+1][WR_END]); CHECK_FAIL("dup");             // muevo al fd 1 el write del pipe 4N+2
            for( i = 3;  i < 4*(qSlaves)+3; i++ )                                       // cierro todos los pipes del 3 hasta 4*qSlaves+3
                close(i);

            char * const paramList[] = {"slave.out", NULL};                             // creacion del slaveN
            execve("slave.out", paramList, 0); CHECK_FAIL("execve");
        }
    }
    //while ( (waitpid(-1, &childStatus, 0)) > 0);


    // escritura en el pipe, se le pasan todos los archivos al primer slave
    char* lineJump = "\n";                                              // delimitador/separador
    for (i = 1; i < argc; i++){
        // ( ! ) tal vez tenga sentido atomizar esto
        write(slavePipes[0][WR_END], argv[i], strlen(argv[i])); CHECK_FAIL("write");
        write(slavePipes[0][WR_END], lineJump, strlen(lineJump)); CHECK_FAIL("write");
    }


    // creacion de archivo de resultados
    int fdResults = open("./results.txt", O_APPEND|O_RDWR|O_CREAT , ALL_PERMISSIONS);
    char resultBuffer[MD5_SIZE];
    int bytesRead;


    // escritura de archivo de resultados, no dinamica con la escritura en el pipe
    for (i = 1; undigestedFiles > 0; i++ ){

        int retSize =  MD5_SIZE+strlen(argv[i])+3;                      // md5sum ret = ( codificacion + "  " + dir + "\n" )
        bytesRead = read(slavePipes[1][RD_END], resultBuffer, retSize);      // lee de [ 5 - readpipe2 ]
        printf("\t\tbytes read : %d\n", bytesRead);     // TESTING
        if ( bytesRead > 0 ){                                           // si hay archivos grandes el slave puede tardar en dejar el resultado
            write(fdResults, resultBuffer, bytesRead); CHECK_FAIL("write");
            undigestedFiles--;
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


*/

