// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "shared.h"


static void handle_error_shared( const char * msg ) {
    perror( msg );
    exit( EXIT_FAILURE );
}

/**
 * Función que crea una memoria compartida en el sistema con el tamaño y nombre especificados.
 * Devuelve una estructura shme_t con información sobre la memoria compartida creada.
 * En caso de error, se llama a handle_error_shared.
 * @param shm_name Nombre de la memoria compartida a crear.
 * @param size Tamaño deseado para la memoria compartida en bytes.
 * @return Estructura shme_t con información sobre la memoria compartida creada.
 */
shme_t shmMake( char * shm_name , int size ){
    shme_t toReturn;
    toReturn.size = size;
    strcpy( toReturn.name , shm_name );

    toReturn.fd = shm_open( shm_name , O_CREAT | O_RDWR , 0666 );    // apertura de la shared memory
    if ( toReturn.fd == -1 ) {
        handle_error_shared( "shm_open failed" );
    }

    int result = ftruncate( toReturn.fd , size );                            // se ajusta el tamaño deseado a la shm
    if (result == -1) {
        handle_error_shared( "ftruncate failed" );
    }

    toReturn.address = mmap( NULL , size , PROT_READ | PROT_WRITE , MAP_SHARED , toReturn.fd , 0 );  // se mapea a la memoria virtual de este proceso
    if ( toReturn.address == MAP_FAILED ) {
        handle_error_shared( "mmap failed" );
    }

    return  toReturn;
}

/**
 * Libera los recursos asociados a la memoria compartida previamente creada.
 * La función desmapea la memoria compartida de la memoria virtual del proceso, elimina la memoria compartida del sistema y cierra el descriptor de archivo asociado.
 * @param shared Puntero a la estructura shme_t que contiene información sobre la memoria compartida a liberar.
 */
void shmDestroy( shme_t * shared ){
    int fd = shared->fd;
    int size = shared->size;
    char *address = shared->address;
    char *name = shared->name;

    int result = munmap( address , size );
    if ( result == -1 ) {
        handle_error_shared( "munmap failed" );
    }

    result = shm_unlink( name );
    if ( result == -1 ) {
        handle_error_shared( "shm_unlink failed" );
    }

    result = close( fd );
    if (result == -1) {
        handle_error_shared( "close failed" );
    }
}
