#include "m&s.h"

// El Esclavo :
// Recibe el/los paths del los archivos a procesar, usa md5sum para hacer el calculo
// Debe tener un pipe por el cual recibe las nuevas tareas, y un pipe a traves del cual envia los resultados obtenidos
// Usa md5sum localizado /usr/bin/md5sum

int main(int argc, char *argv[])
{
    int status;
    char buffer[MD5_SIZE];
    // while ( read(0, buffer, md5size ) ) { }       // lee del pipe hasta que se cae
    read(0, buffer, MD5_SIZE );
    int forkStatus = fork();
    if ( forkStatus != 0 ){                 // FATHER
        waitpid(-1, &status, 0);
    }else{                                  // CHILD
        char * const paramList[] = {"/usr/bin/md5sum", buffer ,NULL};
        execve("/usr/bin/md5sum", paramList, 0); CHECK_FAIL("execve");
    }

    return 0;
}


