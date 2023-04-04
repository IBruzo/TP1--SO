#include "m&s.h"

// El Esclavo :
// Recibe el/los paths del los archivos a procesar, usa md5sum para hacer el calculo
// Debe tener un pipe por el cual recibe las nuevas tareas, y un pipe a traves del cual envia los resultados obtenidos
// Usa md5sum localizado /usr/bin/md5sum

int main(int argc, char *argv[])
{
    char buffer[MAX_PATH_SIZE];
    read(0,buffer,MAX_PATH_SIZE*3); // cada slave recibe 3 archivos, esto pasa a ser un while luego

    const char *s = "\n"; char *token;
    token = strtok(buffer, s);

    int childStatus;
    while( token != NULL ) {
        int forkStatus = fork();
        if ( forkStatus == 0 ){     // Hijo
            char * const paramList[] = {"/usr/bin/md5sum", token ,NULL};
            execve("/usr/bin/md5sum", paramList, 0); CHECK_FAIL("execve");
        }
        token = strtok(NULL, s);
    }
    while ( waitpid(-1, &childStatus, 0) > 0);

    return 0;
}

/*
    Ariel habia dicho que podiamos usar el codigo del consumer que es este de aca abajo, a mi no me anduvo y use strtok, si alguien lo puede
    hacer funcionar, avanti

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    int linecount = 0;

    while ((linelen = getline(&line, &linecap, stdin )) > 0) {
        linecount++;
        printf("Line %d: ", linecount);
        fwrite(line, linelen, 1, stdout);
    }
*/

