#include "m&s.h"
#define MAXBUFFER 4096

void errorHandler(char* message);

int main(int argc, char * argv[]){
    char * lifeLine = "Im still alive";
    int shm_fd; // Aquí guardaremos el file descriptor de la shared memory
    char *shm_ptr;
    int prev_length = 0;
    //sem_t * sem;            // counting semaphore
    write(1, lifeLine, strlen(lifeLine));


    if(argc==3/*4*/){ // si se ejecuta despues se recibe el nombre de la shm y el sem, tambien el tamaño de la shm mem?
        write(1, lifeLine, strlen(lifeLine));
        shm_fd = shm_open(SHM_NAME, O_RDWR, 0666); CHECK_FAIL("shm_open"); // apertura de la shm que creo el master
        shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); CHECK_FAIL("mmap");  // se mapea a la vm
        // sem = sem_open(argv[2], O_CREAT, 0666, 1); // chequear inicializacion en 1, SIN SEMAFORO POR AHORA
        int i;
        for ( i = 0; i < argv[2]; i++ ){ // ESTO TAMBIEN CAMBIA
            int length = strlen(shm_ptr);
            write(1, shm_ptr, length);
        }
        // sem_close(sem);


    }else if(argc==1){ //si se ejecuta con el pipe, tiene que leer del pipe los dos parametros que necesita

        while(prev_length < SHM_SIZE){
            //sem_wait(sem);
            // down(&canRead);

            int length = strlen(shm_ptr);
            write(1, shm_ptr, length);
            //if (length > prev_length) {
            //    printf("%s", shm_ptr + prev_length);  // Print new data to stdout
            //    prev_length = length;
            //}
            //sem_post(sem);
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
