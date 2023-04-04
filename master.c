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

int main(int argc, char *argv[])
{
    printf("\t\tArguments received : %d\n", argc-1);                    // TESTING

    errno = 0;                                                          // seteo en 0 para manejo de errores

    int undigestedFiles = argc - 1;

    // creacion de pipes
    int pipefd_1[2];                                                    // pipe a traves del cual se transmiten los archivos
    int pipefd_2[2];                                                    // pipe a traves del cual se trasnmiten los resultados
    pipe(pipefd_1); CHECK_FAIL("pipe");
    pipe(pipefd_2); CHECK_FAIL("pipe");


    // escritura en el pipe de las direcciones, no dinamica con la lectura del pipe todavia
    char* lineJump = "\n";                                              // delimitador/separador
    int i = 1;
    for (; i < argc; i++ ){
        // ( ! ) tal vez tenga sentido atomizar esto
        write(pipefd_1[WR_END], argv[i], strlen(argv[i])); CHECK_FAIL("write");
        write(pipefd_1[WR_END], lineJump, strlen(lineJump)); CHECK_FAIL("write");
    }


    // llamado a esclavo
    int childStatus;
    int forkStatus = fork(); CHECK_FAIL("fork");
    if ( forkStatus != 0 ){                                             // Padre
        waitpid(-1, &childStatus, 0);                                   // este wait es una cuestion, se pierde todo el paralelismo, el select tut resuelve la race condition que se genera cuando se saca esto ( creo )
    }else{                                                              // Hijo
        close(STDIN); dup(pipefd_1[RD_END]);                            // muevo el readpipe1
        close(STDOUT); dup(pipefd_2[WR_END]);                           // muevo el writepipe2
        close(pipefd_1[RD_END]); close(pipefd_1[WR_END]);               // cierro los pipes extras
        close(pipefd_2[RD_END]); close(pipefd_2[WR_END]);

        char * const paramList[] = {"slave.out", NULL};
        execve("slave.out", paramList, 0); CHECK_FAIL("execve");
    }


    // creacion de archivo de resultados
    int fdResults = open("./results.txt", O_APPEND|O_RDWR|O_CREAT , ALL_PERMISSIONS);
    char resultBuffer[MD5_SIZE];
    int bytesRead;


    // escritura de archivo de resultados, no dinamica con la escritura en el pipe
    for (i = 1; undigestedFiles > 0; i++ ){
        int retSize =  MD5_SIZE+strlen(argv[i])+3;                      // md5sum ret = ( codificacion + "  " + dir + "\n" )
        bytesRead = read(pipefd_2[RD_END], resultBuffer, retSize);      // lee de [ 5 - readpipe2 ]
        if ( bytesRead > 0 ){                                           // si hay archivos grandes el slave puede tardar en dejar el resultado
            printf("\t\tbytes read : %d\n", bytesRead);     // TESTING
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
- revisar si hay permisos demas en el open
*/
