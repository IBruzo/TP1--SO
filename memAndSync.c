#include "memAndSync.h"



/**
 * Función que crea un semáforo con el nombre especificado y devuelve una estructura sema_t que lo representa.
 * @param sem_name: nombre del semáforo.
 * @return sema_t: estructura que contiene el nombre del semáforo y el descriptor de archivo asociado.
*/
sema_t sem_create(char * sem_name){
    sema_t toReturn={0};
    strcpy(toReturn.name,sem_name);
    toReturn.access = sem_open(toReturn.name, O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH, 1);             // se crea/obtiene el fd del semaforo
    if (toReturn.access == SEM_FAILED) {
        handle_error("sem_open failed");
    }
    return toReturn;
}

/**
 * Función que cierra y elimina el semáforo representado por la estructura sema_t.
 * @param sem: puntero a la estructura sema_t que contiene el nombre y el descriptor de archivo asociado al semáforo.
*/
void sem_finish(sema_t * sem){
    int result = sem_close(sem->access);
    if (result == -1) {
        handle_error("sem_close failed");
    }

    result = sem_unlink(sem->name);
    if (result == -1) {
        handle_error("sem_unlink failed");
    }
}

void sem_close2(sema_t * sem){
    int result = sem_close(sem->access);
    if (result == -1) {
        handle_error("sem_close failed");
    }
}

shme_t shm_make(char * shm_name ,int size){
    shme_t toReturn;
    toReturn.size=size;
    strcpy(toReturn.name,shm_name);

    toReturn.fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);    // apertura de la shared memory
    if (toReturn.fd == -1) {
        handle_error("shm_open failed");
    }

    int result = ftruncate(toReturn.fd, size);                            // se ajusta el tamaño deseado a la shm
    if (result == -1) {
        handle_error("ftruncate failed");
    }

    toReturn.address= mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, toReturn.fd, 0);  // se mapea a la memoria virtual de este proceso
    if (toReturn.address == MAP_FAILED) {
        handle_error("mmap failed");
    }

    return  toReturn;
}

void shm_destory(shme_t * shared){
    int fd = shared->fd;
    int size = shared->size;
    char *address = shared->address;
    char *name = shared->name;

    int result = munmap(address, size);
    if (result == -1) {
        handle_error("munmap failed");
    }

    result = shm_unlink(name);
    if (result == -1) {
        handle_error("shm_unlink failed");
    }

    result = close(fd);
    if (result == -1) {
        handle_error("close failed");
    }
}