#include "m&s.h"

// El Master :
// Recibe los directorios de los archivos cuyos md5 se desea calcular ( no preocuparse por permisos o directorios invalidos )
// Inicia los procesos esclavos ( cantidad decidida arbitrariamente )
// Gestiona la cola de trabajo de los esclavos
// Recibe y almacena el resultado de cada esclavo en un buffer por orden de llegada
// Al ser inicializado debe esperar la aparición del proceso Vista, caso positivo debe compartir el buffer con dicho proceso
// Guarda el resultado en un archivos Results.txt

// Last update, corre "./master.out README.md" ( README.md es el archivo que se esta codificando por medio del md5 )
// Devuelve por en un archivo "md5sum : 6af203164b9a2dc9dbbbbcf15edb0331"

int main(int argc, char *argv[])
{
    errno = 0;                      // seteo en 0 para manejo de errores

    if (argc != 4) { printf("Invalid amount of arguments, should only receive 3 Paths\n "); exit(1) ;} // will die

    int pipefd_1[2];                // pipe a traves del cual se transmiten los archivos
    int pipefd_2[2];                // pipe a traves del cual se trasnmiten los resultados

    pipe(pipefd_1); CHECK_FAIL("pipe");
    pipe(pipefd_2); CHECK_FAIL("pipe");

    // fd master =  { [ 0 - stdin ], [ 1 - stdout ], [ 2 - stderr ], [ 3 - readpipe1 ], [ 4 - writepipe1 ], [ 5 - readpipe2 ], [ 6 - writepipe2 ] }
    // fd slave  =  { [ 3 - readpipe1 ], [ 6 - writepipe2 ], [ 2 - stderr ] }  (( OBJETIVO ))

    write(pipefd_1[1], argv[1], strlen( argv[1])); CHECK_FAIL("write");// pasa a ser un ciclo for
    //write(pipefd_1[1], argv[2], strlen( argv[2])); CHECK_FAIL("write");
    //write(pipefd_1[1], argv[3], strlen( argv[3])); CHECK_FAIL("write");

    int status, forkStatus = fork(); CHECK_FAIL("fork");
    if ( forkStatus != 0 ){  // FATHER
        waitpid(-1, &status, 0);
    }else{              // CHILD
        close(0); dup(3);                           // muevo el readpipe1
        close(1); dup(6);                           // muevo el writepipe2
        close(3); close(4); close(5); close(6);     // cierro todo lo innecesario

        char * const paramList[] = {"slave.out", NULL};
        execve("slave.out", paramList, 0); CHECK_FAIL("execve");
    }

    // lee del pipe
    char resultBuffer[MD5_SIZE];
    int bytesRead = read(5, resultBuffer, MD5_SIZE);   // lee de [ 5 - readpipe2 ]
    if (bytesRead != MD5_SIZE) printf("Bytes read differ from md5sum's return size\n"); // tal vez innecesario

    // crea el archivo
    int fdResults = open("./results.txt", O_RDWR|O_CREAT , ALL_PERMISSIONS);
    write(fdResults, resultBuffer, MD5_SIZE); CHECK_FAIL("write");

    return 0;
}

/*
Funcionamiento ejemplificado de master y slave

- se ejecuta el comando en bash con 100 archivos
- el master recibe los archivos, 100 archivos
- decide cuantos esclavos quiere usar, 5 esclavos
- cada esclavo esclavo recibe la cantidad de archivos que puede digerir, 5 archivos
- a medida que cada esclavo termina su tarea el master le vuelve a introducir una nueva tanda

- se manda inicialmente una cantidad >1 de archivos y luego se manda de a uno cada vez que se libera
*/

/*
Posibles improvements y tips :
- lo mas practico es que cada slave tome un solo archivo para maximizar el paralelismo
- usar perror() con el seteo de errno
- referenciar directamente los pipes en vez de usar los numeros de los fds, menos magic numbers
- el master es el unico que sabe de la existencia del archivo
- hay que closear todos los fd abiertos al final, cerrar el w-end del pipe envia un eof y se termina el while de lectura
- combinar la idea de shared memory y results.txt, esta en la clase del 3/4
- toda comunicacion entre master y slave debe ser a traves del pipe ( lo menciono en referencia al pasaje del factor inicial de los slaves )
- envias tareas al slave cada vez este ocioso ( ocioso es un concepto del master, si le envio dos y recibo dos resultados es porque ya esta ocioso )
- getline() para comer tareas del pipe
- hay que tener cuidado con el read de los pipes del master, se usa un select o select_tut que monitorea las activaciones de distintas fds, explicado al finald e la clase del 3/4
- sobre el select : el select se acciona, loopeas en los fds, encontras el que hizo saltar el select ( ISSET  macro ) y procedes a hacer el read al que da TRUE ( no hacer dos seguidos y puede retornar el eof ), reinicializar los fds en cada loop
- hay cosas hechas con syscalls que podrian hacerse con libreria de c, en especial manejo de archivos
*/
