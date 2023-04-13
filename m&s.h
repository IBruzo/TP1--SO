#ifndef MYHEADER_H
#define MYHEADER_H

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
#include <sys/select.h>     // select
#include <math.h>           // ceil
#include <sys/mman.h>       // shm_open, mmap, shm_unlink
#include <semaphore.h>      // sem_open, sem_wait, sem_post

#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define ALL_PERMISSIONS 0x00777
#define MD5_SIZE 32
#define MAX_PATH_SIZE 1024
#define CHECK_FAIL(functionName) ((errno != 0) ? (perror(functionName), exit(1)) : 0)
#define SEM_NAME "countingSemaphore"
#define SHM_NAME "/shalom"
#define SHM_SIZE 2048 //tamaño arbitrario

typedef int sem;
sem canRead = 0;
sem empty = SHM_SIZE;

// typedef struct
// {
//     sem_t *access;
//     char name[NAME_MAX - 4];
// } sem_t;


#endif
