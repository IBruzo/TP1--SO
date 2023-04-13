all: master slave vista

# poner std=c99 en compilacion hace que no se permita usar getline >:(
master: master.c
	gcc -Wall -std=c99 -lm -lrt -pthread $< -o $@.out

slave: slave.c
	gcc -Wall -std=c99 -lm -lrt -pthread $< -o $@.out

vista: vista.c
	gcc -Wall -std=c99 -lm -lrt -pthread $< -o $@.out


clean:
	rm -f master.out slave.out vista.out results.txt

.PHONY: all clean
