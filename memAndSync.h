#ifndef MEMANDSYNC_H
#define MEMANDSYNC_H

#include <stdio.h>          // printf
#include <sys/types.h>      // fork, waitpid, open, ftruncate
#include <unistd.h>         // fork, execve, pipe, dup, close, read, ftruncate
#include <sys/wait.h>       // waitpid
#include <stdlib.h>         // exit
#include <string.h>         // strlen
#include <sys/stat.h>       // open, shm_open, shm_unlink, sem_open
#include <fcntl.h>          // open, shm_open, shm_unlink, sem_open
#include <errno.h>          // errno
#include <string.h>         // strcat, memset
#include <sys/mman.h>       // shm_open, mmap, shm_unlink
#include <semaphore.h>      // sem_open, sem_wait, sem_post

#define CHECK_FAIL(functionName) ((errno != 0) ? (perror(functionName), exit(1)) : 0)
#define MAX_NAME 30
/*
TODO create shm close shm, create smfaro y close usar un struct??
*/
typedef struct
{
    char name[MAX_NAME];
    int fd;        // used to read and write from the memory
    int size;      // the size of the memory assigned
    char *address; // use to map from a new process
} shme_t;

typedef struct
{
    sem_t * access;
    char name[MAX_NAME];
} sema_t;

sema_t sem_create(char * sem_name);
void sem_finish(sema_t * sem);

shme_t shm_make(char * shm_name ,int size);
void shm_destory(shme_t * shared);


#endif