all: semaforo shared master slave vista

# poner std=c99 en compilacion hace que no se permita usar getline >:(
semaforo: semaforo.c
	gcc -Wall -std=c99 -lm -lrt -pthread  -D_XOPEN_SOURCE=500 $< -c

shared: shared.c
	gcc -Wall -std=c99 -lm -lrt -pthread  -D_XOPEN_SOURCE=500 $< -c


master: master.c 
	gcc -Wall -std=c99 -lm -lrt -D_XOPEN_SOURCE=500 -pthread semaforo.o shared.o $< -o $@.out

slave: slave.c 
	gcc -Wall -std=c99 -lm -lrt  -pthread  shared.o $<  -o $@.out

vista: vista.c 
	gcc -Wall -std=c99 -lm -lrt -D_XOPEN_SOURCE=500 -pthread semaforo.o shared.o $< -o $@.out


clean:
	rm -f master.out slave.out vista.out  results.txt 

.PHONY: all clean
