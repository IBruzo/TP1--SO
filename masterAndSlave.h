#ifndef MYHEADER_H
#define MYHEADER_H

#include <stdio.h>          /*  printf                                           */   
#include <sys/types.h>      /*  fork, waitpid, open, ftruncate                   */                           
#include <unistd.h>         /*  fork, execve, pipe, dup, close, read, ftruncate  */                                            
#include <sys/wait.h>       /*  waitpid                                          */    
#include <stdlib.h>         /*  exit                                             */ 
#include <string.h>         /*  strlen                                           */   
#include <sys/stat.h>       /*  open, shm_open, shm_unlink, sem_open             */                                 
#include <fcntl.h>          /*  open, shm_open, shm_unlink, sem_open             */                                 
#include <errno.h>          /*  errno                                            */  
#include <string.h>         /*  strcat, memset                                   */           
#include <sys/select.h>     /*  select                                           */   
#include <math.h>           /*  ceil                                             */ 
#include <sys/mman.h>       /*  shm_open, mmap, shm_unlink                       */                       
#include <semaphore.h>      /*  sem_open, sem_wait, sem_post                     */                         
#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define ALL_PERMISSIONS 0x00777        /* permisos de lectura, escritura y ejecución */
#define MD5_SIZE 32
#define MAX_PATH_SIZE 2048             /* tamaño arbitrario */
#define SEM_NAME "countingSemaphore"   /* nombre arbitrario */
#define SHM_NAME "/shalom"             /* nombre arbitrario */
#define SHM_SIZE 65536                 /* tamaño arbitrario */
#define MAX_BUFFER_SIZE 65536          /* tamaño arbitrario */


/**
 * Función que maneja errores. Recibe un mensaje de error como parámetro y lo imprime utilizando la función perror.
 * Además, finaliza la ejecución del programa con un código de error utilizando la función exit.
 * @param msg Mensaje de error a imprimir.
 */
void handle_error( const char * msg ) {
    perror( msg );
    exit( EXIT_FAILURE );
}


#endif
