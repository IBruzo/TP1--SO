#include <stdio.h>          // printf
#include <sys/types.h>      // fork, waitpid
#include <unistd.h>         // fork, execve, pipe, dup, close, read
#include <sys/wait.h>       // waitpid
#include <stdlib.h>         // exit
#include <string.h>         // strlen

#define md5size 32

// El Master :
// Recibe los directorios de los archivos cuyos md5 se desea calcular ( no preocuparse por permisos o directorios invalidos )
// Inicia los procesos esclavos ( cantidad decidida arbitrariamente )
// Gestiona la cola de trabajo de los esclavos
// Recibe y almacena el resultado de cada esclavo en un buffer por orden de llegada
// Al ser inicializado debe esperar la aparición del proceso Vista, caso positivo debe compartir el buffer con dicho proceso
// Guarda el resultado en un archivos Results.txt

int main(int argc, char *argv[])
{

    if ( argc != 2 ) { printf("Invalid amount of arguments, should only receive a Path\n "); exit(1) ;}

    int pipefd_1[2];                // pipe a traves del cual se transmiten los archivos
    int pipefd_2[2];                // pipe a traves del cual se trasnmiten los resultados

    if ( pipe(pipefd_1) == -1 ) {printf("Error whilst creating anonymous pipe\n"); exit(1);}
    if ( pipe(pipefd_2) == -1 ) {printf("Error whilst creating anonymous pipe\n"); exit(1);}
    // fd master =  { [ 0 - stdin ], [ 1 - stdout ], [ 2 - stderr ], [ 3 - readpipe1 ], [ 4 - writepipe1 ], [ 5 - readpipe2 ], [ 6 - writepipe2 ] }
    // fd slave  =  { [ 3 - readpipe1 ], [ 6 - writepipe2 ], [ 2 - stderr ] }  (( OBJETIVO ))

    if ( write(pipefd_1[1], argv[1], strlen(argv[1])) == -1 ){ printf("Pipe writing failed\n"); exit(1);}
    int status;
    int forkStatus = fork();
    if ( forkStatus != 0 ){  // FATHER
        waitpid(-1, &status, 0);
    }else{              // CHILD    // tal vez es mejor estilo referenciar el array del pipe
        close(0); dup(3);                           // muevo el readpipe1
        close(1); dup(6);                           // muevo el writepipe2
        close(3); close(4); close(5); close(6);     // cierro todo lo innecesario

        char * const paramList[] = {"slave.out", NULL};
        if ( execve("slave.out", paramList, 0) == -1 ) printf("Execve Failed") ;
        exit(1);
    }

    char resultBuffer[md5size];
    int bytesRead = read( 5, resultBuffer, md5size );   // lee de [ 5 - readpipe2 ]
    if ( bytesRead != md5size ) printf( " Bytes read differ from md5sum's return size\n");
    printf("md5sum : %s\n", resultBuffer);
    // deberia writear los paths que voy queriendo tras inicializar los procesos, y los procesos deberian estar leyendo constantemente del
    // otro lado del pipe
    return 0;
}
