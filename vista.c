#include "masterAndSlave.h"


int main(int argc, char * argv[])
{
    char * welcomeMessage = "Waiting for master...\n";
    int shm_fd;                                             // file descriptor de la shared memory
    char * shm_ptr;                                         // puntero a la shared memory
    sem_t * sem;                                            // counting semaphore
    int amount = 0;
    if( write( STDOUT_FILENO , welcomeMessage , strlen( welcomeMessage ) ) == -1 ){
        handle_error("write failed");
    }

    shm_fd = shm_open( SHM_NAME , O_CREAT | O_RDWR , 0666 );    // apertura de la shared memory
    if( shm_fd == -1 ){
        handle_error( "shm_open failed vista" );
    }

    if( ftruncate( shm_fd , SHM_SIZE ) == -1 ){                                             // se ajusta el tamaño deseado a la shm
        handle_error( "ftruncate failed vista" );
    }

    shm_ptr = mmap( NULL , SHM_SIZE , PROT_READ | PROT_WRITE , MAP_SHARED , shm_fd , 0 );  // se mapea a la memoria virtual de este proceso
     if ( shm_ptr == MAP_FAILED ) {
        handle_error( "mmap failed vista" );
    }

    sem = sem_open( SEM_NAME , O_CREAT , 0666 , 0 );             // se crea/obtiene el fd del semaforo
    if ( sem == SEM_FAILED ) {
        handle_error( "sem_open failed vista" );                                                       // prints the error message to stderr
    }


    if( argc == 2 ){                                            // caso donde se corre vista aparte

        amount = atoi( argv[1] );

    }else if( argc == 1 ){                                      // caso donde se corre con el pipe

        char readBuffer[ MAX_PATH_SIZE ];
        if( read( 0 , readBuffer , MAX_PATH_SIZE ) == -1 ){
            handle_error( "read failed vista" );
        }

        amount = atoi( readBuffer );

    }else{
        handle_error( "Wrong amount of parameters" );
    }

    

    int offset = 0;
    for ( size_t i = 0 ; i < amount ; i++ ){
        sem_wait( sem ); 
        //printf("OFFSET %d\n", offset);
        //printf("SHARED MEMORY ENTERA : %s\n", shm_ptr);
       
        size_t msgLen = strlen(shm_ptr + offset);
        if( write( STDOUT_FILENO , shm_ptr  + offset  ,msgLen ) == -1 ){
            handle_error( "write failed vista" );
        }        // // escritura en stdout
        offset += msgLen;
    }
    
    // cerrar semaforo
    if ( sem_close( sem ) == -1 ) {
        handle_error( "sem_close failed vista" );
    }

    // Unlink shared memory
    if ( munmap( shm_ptr , SHM_SIZE ) == -1 ) {
        handle_error( "munmap failed vista" );
    }

    if ( close( shm_fd ) == -1 ){
        handle_error( "close failed vista" );
    }
    return 0;
}
