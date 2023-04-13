#include "m&s.h"


int main(int argc, char * argv[])
{
    char * lifeLine = "Im still alive\n";
    int shm_fd; // Aquí guardaremos el file descriptor de la shared memory
    char * shm_ptr;
    int prev_length = 0;
    sem_t * sem;         // counting semaphore 
    
    write(STDOUT, lifeLine, strlen(lifeLine));
    
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666); // apertura de la shm que creo el master
    CHECK_FAIL("shm_open"); 

    ftruncate(shm_fd, SHM_SIZE); // Darle el tamaño deseado a la shm
    CHECK_FAIL("ftruncate");

    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); // se mapea a la vm
    CHECK_FAIL("mmap");  

    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1); // chequear inicializacion en 1, SIN SEMAFORO POR AHORA

    if(argc==2){ // si se ejecuta despues se recibe el nombre de la shm y el sem, tambien el tamaño de la shm mem?
        write(STDOUT, lifeLine, strlen(lifeLine));
        int i;
        
        int length = strlen(shm_ptr);


        sem_wait(sem);
        write(STDOUT, shm_ptr, MAX_BUFFER_SIZE);
        sem_wait(sem);
        write(STDOUT, shm_ptr, MAX_BUFFER_SIZE);
        sem_close(sem);


    }else if(argc==1){ //si se ejecuta con el pipe, tiene que leer del pipe los dos parametros que necesita
        char readBuffer[MAX_PATH_SIZE];
        read(STDIN, readBuffer, MAX_PATH_SIZE-1);
        const char *s = "\n";
        char *token;
        token = strtok(readBuffer, s);
        int childStatus;
        while( token != NULL ) {
            write(STDOUT, token, strlen(token));
            token = strtok(NULL, s);
       
        }
        memset( readBuffer, 0, sizeof(readBuffer));


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
        perror("wrong ammount of parameters");
        exit(1);
    }
}

