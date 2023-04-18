// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "masterAndSlave.h"
#include "semaforo.h"
#include "shared.h"

int main( int argc , char * argv[] )
{
    char * welcomeMessage = "Waiting for master...\n";
    if( write( STDOUT_FILENO , welcomeMessage , strlen( welcomeMessage ) ) == -1 ){
        handle_error("write failed");
    }


    shme_t shmPtr = shmMake( SHM_NAME , SHM_SIZE );
    sema_t sem = semCreate( SEM_NAME );

    /*  Cantidad de archivos a leer  */
    int amount = 0;

    /*  Caso donde se corre vista aparte  */          
    if ( argc == 2 ){                                                                                        

        amount = atoi( argv[1] );

    } else if ( argc == 1 ){   /*  Caso donde se corre con el pipe */                                                                                        

        char readBuffer[ MAX_PATH_SIZE ];
        if( read( 0 , readBuffer , MAX_PATH_SIZE ) == -1 ){
            handle_error( "read failed vista" );
        }

        amount = atoi( readBuffer );

    } else {
        handle_error( "Wrong amount of parameters" );
    }

    

    int offset = 0;
    for ( size_t i = 0 ; i < amount+1 ; i++ ){
        if(sem_wait( sem.access ) == -1){
            handle_error( "sem_wait failed vista" );
        } 
       
        size_t msgLen = strlen( shmPtr.address + offset );

        /* escritura en stdout */
        if( write( STDOUT_FILENO , shmPtr.address  + offset  , msgLen ) == -1 ){                      
            handle_error( "write failed vista" );
        }        
        offset += msgLen;
    }
    

    //sem_closeWrap(sem);
    /* cerrar semaforo */
    sem_close(sem.access);

    /* Unlink shared memory */
    if ( munmap( shmPtr.address , SHM_SIZE ) == -1 ) {
        handle_error( "munmap failed vista" );
    }

    /*Close shared memory */
    if ( close( shmPtr.fd ) == -1 ){
        handle_error( "close failed vista" );
    }

    return 0;
}
