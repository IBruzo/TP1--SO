// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "semaforo.h"

static void handle_error_semaforo( const char * msg ) {
    perror( msg );
    exit( EXIT_FAILURE );
}

/**
 * Función que crea un semáforo con el nombre especificado y devuelve una estructura sema_t que lo representa.
 * @param sem_name: nombre del semáforo.
 * @return sema_t: estructura que contiene el nombre del semáforo y el descriptor de archivo asociado.
*/
sema_t semCreate( char * sem_name ){
    sema_t toReturn = { 0 };
    strcpy( toReturn.name,sem_name );
    toReturn.access = sem_open( toReturn.name , O_CREAT , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH , 1 );             // se crea/obtiene el fd del semaforo
    if ( toReturn.access == SEM_FAILED ) {
        handle_error_semaforo( "sem_open failed" );
    }
    return toReturn;
}

/**
 * Función que cierra y elimina el semáforo representado por la estructura sema_t.
 * @param sem: puntero a la estructura sema_t que contiene el nombre y el descriptor de archivo asociado al semáforo.
*/
void semFinish( sema_t * sem ){
    int result = sem_close( sem->access );
    if ( result == -1 ) {
        handle_error_semaforo( "sem_close failed" );
    }

    result = sem_unlink( sem->name );
    if ( result == -1 ) {
        handle_error_semaforo( "sem_unlink failed" );
    }
}

