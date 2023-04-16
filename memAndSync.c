#include "memAndSync.h"



/**
 * Función que crea un semáforo con el nombre especificado y devuelve una estructura sema_t que lo representa.
 * @param sem_name: nombre del semáforo.
 * @return sema_t: estructura que contiene el nombre del semáforo y el descriptor de archivo asociado.
*/
sema_t sem_create(char * sem_name){
    sema_t toReturn={0};
    strcpy(toReturn.name,sem_name);
    toReturn.access=sem_open(toReturn.name, O_CREAT, 0666, 1);             // se crea/obtiene el fd del semaforo
    CHECK_FAIL("sem_open");
    return toReturn;
}

/**
 * Función que cierra y elimina el semáforo representado por la estructura sema_t.
 * @param sem: puntero a la estructura sema_t que contiene el nombre y el descriptor de archivo asociado al semáforo.
*/
void sem_finish(sema_t * sem){
    sem_close(sem->access);
    CHECK_FAIL("sem-close");

    sem_unlink(sem->name);
    CHECK_FAIL("sem_unlink");
}

shme_t shm_make(char * shm_name ,int size){
    shme_t toReturn;
    toReturn.size=size;
    strcpy(toReturn.name,shm_name);

    toReturn.fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);    // apertura de la shared memory
    CHECK_FAIL("shm_open");

    ftruncate(toReturn.fd, size);                            // se ajusta el tamaño deseado a la shm
    CHECK_FAIL("ftruncate");

    toReturn.address= mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, toReturn.fd, 0);  // se mapea a la memoria virtual de este proceso
    CHECK_FAIL("mmap");

    return  toReturn;
}

void shm_destory(shme_t * shared){
    int fd = shared->fd;
    int size = shared->size;
    char *address = shared->address;
    char *name = shared->name;

    munmap(address, size);
    CHECK_FAIL("mummap");

    shm_unlink(name);
    CHECK_FAIL("shm_unlink");

    close(fd);
    CHECK_FAIL("close");
}