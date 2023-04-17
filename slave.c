#include "masterAndSlave.h"

/* 
 * Esclavo :                                                                                                                  
 * Recibe el/los paths del los archivos a procesar, usa md5sum para hacer el calculo                                                                                                       
 * Debe tener un pipe por el cual recibe las nuevas tareas, y un pipe a traves del cual envia los resultados obtenidos                                                                                                       
 * Usa md5sum localizado /usr/bin/md5sum                                                                                                       
 */

int main( int argc , char * argv[] )
{
    int childStatus;
    char buffer[ MAX_PATH_SIZE ];

    /* Buffer con el slavePid */
    int slavepid = getpid();
    char bufferPID[ MAX_BUFFER_SIZE ];
    snprintf( bufferPID, sizeof( bufferPID ) , "%d" , slavepid );

    /* Se lee del buffer hasta que el master tire abajo el pipe y se reciba el EOF */
    while ( ( read(0,buffer,MAX_PATH_SIZE) ) > 0 ){

        /* Se utiliza un buffer alternativo para no alterar el buffer del pipe */
        char bufferCopy[ MAX_PATH_SIZE ];
        strncpy( bufferCopy , buffer , MAX_PATH_SIZE );

        /*
        Luego de la carga estatica el buffer puede tener varios archivos
            buffer = "Archivo1.txt\nArchivo2.txt\nArchivo3.txt\n"
            - entra al while
            token = "Archivo1.txt"
            token = "Archivo2.txt"
            token = "Archivo3.txt"
        Cuando comienza la carga dinamica, siempre se carga de a 1
            buffer = "Archivo4.txt\n"
            token = "Archivo4.txt"
        */

        const char * s = "\n";
        char * token;
        token = strtok( bufferCopy , s );

        while ( token != NULL ) {
            int pipefd[ 2 ];
            if ( pipe( pipefd ) == -1 ){
                handle_error( "pipe failed" );
            }
            /* fdSlave = { [ 0 - readpipe1 ], [ 1 - writepipe2 ], [ 2 - stderr ], [ 3 - pipeRD(pipefd[0]) ], [ 4 - pipeWR(pipefd[1]) ] } */
            int pid = fork();
            if ( pid == -1 ){
                handle_error( "fork failed" );
            }
            if ( pid == 0 ){
                /* Ordeno los fds -> fdSlaveSon = { [ 0 - readpipe1 ], [ 4 - pipeWR(pipefd[1]) ], [ 2 - stderr ] }  */ 
                close( STDOUT_FILENO );
                dup( pipefd[ 1 ] );
                close( pipefd[ 0 ] );
                close( pipefd[ 1 ] );

                /* La salida de md5sum va al buffer del pipe que comparte el slaveSon y Slave */
                char * const paramList[] = { "/usr/bin/md5sum" , token , NULL };
                if ( execve( "/usr/bin/md5sum" , paramList , 0 ) == -1 ){
                    handle_error( "execve failed" );
                }

            } else {
                /* Cierro el unico fd innecesario -> fdSlave = { [ 0 - readpipe1 ], [ 1 - writepipe2 ], [ 2 - stderr ], [ 3 - pipeRD(pipefd[0]) ] } */
                close( pipefd[ 1 ] );

                /* Creo el buffer donde se almacena el resultado del md5, obtenido por medio del pipe */
                char md5Output[ MAX_BUFFER_SIZE ];
                int numRead;
                while ( ( numRead = read(pipefd[0] , md5Output , MAX_PATH_SIZE ) ) > 0 );
                if ( numRead == -1 ){
                    handle_error( "read failed slave" );
                }

                /* Printeo por el stdout que al estar en el slave es el pipe de escritura del pipe que lo conecta con el master */
                printf( "Slave pid : %s\t%s" , bufferPID , md5Output );

                /* Limpio el stdout para la proxima ejecucion */
                fflush( stdout );

                /* Cierro el pipe extra */
                close( pipefd[0] );

                /* Esta logica tiene que estar sincronizada */
                waitpid( pid, &childStatus , 0 );
            }
            token = strtok( NULL, s );
        }

        /* Limpio el buffer */
        memset( buffer , 0 , sizeof( buffer ) );
    }

    return 0;
}
