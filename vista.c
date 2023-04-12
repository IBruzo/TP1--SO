#include "m&s.h"
#define MAXBUFFER 4096

void errorHandler(char* message);


//
int main(int argc, char * argv[]){

    int shm_fd; // Aquí guardaremos el file descriptor de la shared memory
    char *shm_ptr;
    int prev_length = 0;

    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);  // open the existing shared memory
    
    if (shm_fd == -1) {
        perror("shm_open");            exit(EXIT_FAILURE);
    }

    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);  // map the shared memory
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if(argc==2){ // si se ejecuta despues
       
    }else if(argc==1){ //si se ejecuta con el pipe
        
        while(prev_length < SHM_SIZE){
            down(&canRead);
            
            int length = strlen(shm_ptr);

            if (length > prev_length) {
                printf("%s", shm_ptr + prev_length);  // Print new data to stdout
                prev_length = length;
            }    
        }

    }else{
        errorHandler("wrong ammount of parameters for process\n");
    }
}

void errorHandler(char* message){
    perror(message);
    exit(1);
}