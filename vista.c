#include "m&s.h"
#define MAXBUFFER 4096

void errorHandler(char* message);

int main(int argc, char * argv[]){

    int shm_fd; // Aquí guardaremos el file descriptor de la shared memory

    char *shm_ptr;
    int prev_length = 0;
    sem_t * sem;            // counting semaphore

    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666); CHECK_FAIL("shm_open"); // open the existing shared memory
    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); CHECK_FAIL("mmap");  // map the shared memory
    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1); // chequear inicializacion en 1

    if(argc==2){ // si se ejecuta despues

    }else if(argc==1){ //si se ejecuta con el pipe

        while(prev_length < SHM_SIZE){
            sem_wait(sem);
            // down(&canRead);

            int length = strlen(shm_ptr);

            if (length > prev_length) {
                printf("%s", shm_ptr + prev_length);  // Print new data to stdout
                prev_length = length;
            }
            sem_wait(sem);
        }

    }else{
        errorHandler("wrong ammount of parameters for process\n");
    }
}

// pasar a una lib
void errorHandler(char* message){
    perror(message);
    exit(1);
}
