#include "masterAndSlave.h"


int main( int argc , char * argv[] )
{
    char * welcomeMessage = "Waiting for master...\n";
    if( write( STDOUT_FILENO , welcomeMessage , strlen( welcomeMessage ) ) == -1 ){
        handle_error("write failed");
    }

    /*  File descriptor de la shared memory */
    int shm_fd;                                                       

    /*  Puntero a la shared memory  */                                                    
    char * shm_ptr;                                      

    /*  Counting semaphore  */                                                                          
    sem_t * sem;       

    /*  Apertura de la shared memory  */
    shm_fd = shm_open( SHM_NAME , O_CREAT | O_RDWR , 0666 );                                                      
    if ( shm_fd == -1 ){
        handle_error( "shm_open failed vista" );
    }

    /*  Se ajusta el tamaño deseado a la shm */
    if ( ftruncate( shm_fd , SHM_SIZE ) == -1 ){                                                                           
        handle_error( "ftruncate failed vista" );
    }

    /*  Se mapea a la memoria virtual de este proceso  */ 
    shm_ptr = mmap( NULL , SHM_SIZE , PROT_READ | PROT_WRITE , MAP_SHARED , shm_fd , 0 );                                         
    if ( shm_ptr == MAP_FAILED ) {
        handle_error( "mmap failed vista" );
    }

    /*  Se crea/obtiene el fd del semaforo */ 
    sem = sem_open( SEM_NAME , O_CREAT , 0666 , 0 );                                                                   
    if ( sem == SEM_FAILED ) {
        handle_error( "sem_open failed vista" );                                                      
    }

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
    for ( size_t i = 0 ; i < amount ; i++ ){
        if(sem_wait( sem ) == -1){
            handle_error( "sem_wait failed vista" );
        } 
       
        size_t msgLen = strlen( shm_ptr + offset );

        /* escritura en stdout */
        if( write( STDOUT_FILENO , shm_ptr  + offset  , msgLen ) == -1 ){                                
            handle_error( "write failed vista" );
        }        
        offset += msgLen;
    }
    
    /* cerrar semaforo */
    if ( sem_close( sem ) == -1 ) {
        handle_error( "sem_close failed vista" );
    }

    /* Unlink shared memory */
    if ( munmap( shm_ptr , SHM_SIZE ) == -1 ) {
        handle_error( "munmap failed vista" );
    }

    /*Close shared memory */
    if ( close( shm_fd ) == -1 ){
        handle_error( "close failed vista" );
    }

    return 0;
}
