#include "m&s.h"

// El Esclavo :
// Recibe el/los paths del los archivos a procesar, usa md5sum para hacer el calculo
// Debe tener un pipe por el cual recibe las nuevas tareas, y un pipe a traves del cual envia los resultados obtenidos
// Usa md5sum localizado /usr/bin/md5sum

/*
int main(int argc, char *argv[])
{


    char buffer[MAX_PATH_SIZE];
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    int linecount = 0;

    while ( (getline(&line, &linecap, stdin )) != EOF ){

        line[strcspn(line, "\n")] = '\0';                                       // les saco el eol
        int childStatus;
        int forkStatus = fork(); CHECK_FAIL("pipe");
        if ( forkStatus == 0 ){
            waitpid(forkStatus, &childStatus, 0);
        }else{                                                                  // hijo
            char * const paramList[] = {"/usr/bin/md5sum", line ,NULL};
            execve("/usr/bin/md5sum", paramList, 0); CHECK_FAIL("execve");
        }
    }
    free(line);
    return 0;
}

*/



int main(int argc, char *argv[])
{
    int childStatus;
    char buffer[MAX_PATH_SIZE]; 

    while ( (read(0,buffer,MAX_PATH_SIZE)) != EOF ){

        char bufferCopy[MAX_PATH_SIZE];
        strncpy(bufferCopy, buffer, MAX_PATH_SIZE);
        
        const char *s = "\n";
        char *token;
        token = strtok(bufferCopy, s);

        while( token != NULL ) {
            int pipefd[2]; // fd 3 y 4
            pipe(pipefd); CHECK_FAIL("pipe");

            pid_t pid = fork();
            if ( pid == 0 ){
                close(pipefd[0]); 
                dup2(pipefd[1], STDOUT_FILENO); 
                close(pipefd[1]); 

                char * const paramList[] = {"/usr/bin/md5sum", token ,NULL};
                execve("/usr/bin/md5sum", paramList, 0);
                CHECK_FAIL("execve");

            }else{
                close(pipefd[1]); 

                char bufferPID[MAX_PATH_SIZE];
                snprintf(bufferPID, sizeof(bufferPID), "%d", pid);

                char md5Output[MAX_PATH_SIZE];
                ssize_t numRead = read(pipefd[0], md5Output, MAX_PATH_SIZE);
                if (numRead == -1) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                md5Output[numRead] = '\0'; 
                printf("Slave pid : %d\t%s%s", getpid(), bufferPID, md5Output);
                fflush(stdout);

                close(pipefd[0]);
                waitpid(pid, &childStatus, 0); 
            }
            token = strtok(NULL, s);
        }
        memset( buffer, 0, sizeof(buffer));
    } 
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
