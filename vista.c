#include "m&s.h"
#define MAXBUFFER 4096

void erroHandler(char* message);

int main(int argc, char * argv[]){

    if(argc==2){ // si se ejecuta despues
       
    }else if(argc==1){ //si se ejecuta con el pipe

    }else{
        erroHandler("worng ammount of parameters for process");
    }
}

void erroHandler(char* message){
    perror(message);
    exit(1);
}