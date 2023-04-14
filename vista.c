#include "m&s.h"


int main(int argc, char * argv[])
{
    char * welcomeMessage = "Waiting for master...\n";
    int shm_fd;                                             // file descriptor de la shared memory
    char * shm_ptr;                                         // puntero a la shared memory
    sem_t * sem;                                            // counting semaphore

    write(STDOUT, welcomeMessage, strlen(welcomeMessage));
    CHECK_FAIL("write");

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);    // apertura de la shared memory
    CHECK_FAIL("shm_open");

    ftruncate(shm_fd, SHM_SIZE);                            // se ajusta el tamaño deseado a la shm
    CHECK_FAIL("ftruncate");

    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);  // se mapea a la memoria virtual de este proceso
    CHECK_FAIL("mmap");

    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);             // se crea/obtiene el fd del semaforo
    CHECK_FAIL("sem_open");

    if(argc==2){                                            // caso donde se corre vista aparte

        for (size_t i = 0; i < atoi(argv[1]); i++){
            sem_wait(sem);
            write(STDOUT, shm_ptr, MAX_BUFFER_SIZE);        // escritura en stdout
            CHECK_FAIL("write");
        }
        sem_close(sem);

    }else if(argc==1){                                      // caso donde se corre con el pipe

        char readBuffer[MAX_PATH_SIZE];
        read(0, readBuffer, MAX_PATH_SIZE);
        CHECK_FAIL("read");

        for (size_t i = 0; i < atoi(readBuffer); i++){
            sem_wait(sem);
            write(STDOUT, shm_ptr, MAX_BUFFER_SIZE);        // // escritura en stdout
            CHECK_FAIL("write");
        }
        sem_close(sem);

    }else{
        perror("Wrong amount of parameters");
        exit(1);
    }
}

