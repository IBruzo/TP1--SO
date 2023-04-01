#include <stdio.h>          // printf
#include <sys/types.h>      // fork, waitpid
#include <unistd.h>         // fork, execve
#include <sys/wait.h>       // waitpid
#include <stdlib.h>         // exit

#define md5size 32
// El Esclavo :
// Recibe el/los paths del los archivos a procesar, usa md5sum para hacer el calculo
// Debe tener un pipe por el cual recibe las nuevas tareas, y un pipe a traves del cual envia los resultados obtenidos
// Usa md5sum localizado /usr/bin/md5sum

int main(int argc, char *argv[])
{
    // if ( argc != 2 ) { printf("Invalid amount of arguments, should only receive a Path "); exit(1) ;} [ PARA TESTEO AISLADO ]
    int status;
    char buffer[md5size + 1];            // creo que el md5sum retorna un codigo de 32 caracteres, not sure
    // while ( read(0, buffer, md5size ) ) { }       // lee del pipe hasta que se cae
    read(0, buffer, md5size );
    int forkStatus = fork();
    if ( forkStatus != 0 ){                 // FATHER
        waitpid( -1, &status, 0 );
    }else{                                  // CHILD
        char * const paramList[] = {"/usr/bin/md5sum", buffer ,NULL};
        if ( execve("/usr/bin/md5sum", paramList, 0) == -1 ) printf("Execve Failed") ;
        exit(1);
    }

    return 0;
}


