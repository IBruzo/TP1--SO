#ifndef MYHEADER_H
#define MYHEADER_H

#include <stdio.h>          // printf
#include <sys/types.h>      // fork, waitpid, open
#include <unistd.h>         // fork, execve, pipe, dup, close, read
#include <sys/wait.h>       // waitpid
#include <stdlib.h>         // exit
#include <string.h>         // strlen
#include <sys/stat.h>       // open
#include <fcntl.h>          // open
#include <errno.h>          // errno
#include <stdio.h>          // getline

#define  _POSIX_C_SOURCE 200809L        // getline
#define ALL_PERMISSIONS 0x00777
#define MD5_SIZE 32
#define CHECK_FAIL(functionName) ((errno != 0) ? (perror(functionName), exit(1)) : 0)

#endif
