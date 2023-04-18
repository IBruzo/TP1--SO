// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#ifndef SEMAFORO_H
#define SEMAFORO_H


#include <stdio.h>          // printf
#include <sys/types.h>      // fork, waitpid, open, ftruncate
#include <unistd.h>         // fork, execve, pipe, dup, close, read, ftruncate
#include <stdlib.h>         // exit
#include <sys/stat.h>       // open, shm_open, shm_unlink, sem_open
#include <fcntl.h>          // open, shm_open, shm_unlink, sem_open
#include <errno.h>          // errno
#include <string.h>         // strcat, memset
#include <sys/mman.h>       // shm_open, mmap, shm_unlink
#include <sys/wait.h>       /*  waitpid                                          */    
#include <sys/select.h>     /*  select                                           */   
#include <math.h>           /*  ceil                                             */ 
#include <semaphore.h>      /*  sem_open, sem_wait, sem_post                     */ 

#define MAX_NAME 30
typedef struct
{
    sem_t * access;
    char name[ MAX_NAME ];
} sema_t;

sema_t semCreate( char * sem_name );
void semFinish( sema_t * sem );

#endif