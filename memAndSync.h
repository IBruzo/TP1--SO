#ifndef MEMANDSYNC_H
#define MEMANDSYNC_H

#include <sys/mman.h>       // shm_open, mmap, shm_unlink
#include <semaphore.h>      // sem_open, sem_wait, sem_post
#include <sys/stat.h>       // open, shm_open, shm_unlink, sem_open
#include <fcntl.h>          // open, shm_open, shm_unlink, sem_open
#include <errno.h>          // errno

#define CHECK_FAIL(functionName) ((errno != 0) ? (perror(functionName), exit(1)) : 0)
/*
TODO create shm close shm, create smfaro y close usar un struct??
*/
#endif
