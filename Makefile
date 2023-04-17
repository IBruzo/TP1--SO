all: memAndSync master slave vista

# poner std=c99 en compilacion hace que no se permita usar getline >:(
memAndSync: memAndSync.c memAndSync.h
	gcc -Wall -std=c99 -lrt -D_XOPEN_SOURCE=500 -pthread $< -c

master: master.c memAndSync.h
	gcc -Wall -std=c99 -lm -lrt -D_XOPEN_SOURCE=500 -pthread memAndSync.o $< -o $@.out

slave: slave.c memAndSync.h
	gcc -Wall -std=c99 -lm -lrt -D_XOPEN_SOURCE=500 -pthread memAndSync.o $<  -o $@.out

vista: vista.c memAndSync.h
	gcc -Wall -std=c99 -lm -lrt -D_XOPEN_SOURCE=500 -pthread memAndSync.o $< -o $@.out


clean:
	rm -f master.out slave.out vista.out  results.txt memAndSync.o

.PHONY: all clean
