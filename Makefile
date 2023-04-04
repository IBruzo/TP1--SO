all: master slave

# poner std=c99 en compilacion hace que no se permita usar getline >:(
master: master.c
	gcc -Wall $< -o $@.out

slave: slave.c
	gcc -Wall $< -o $@.out


clean:
	rm -f master.out slave.out

.PHONY: all clean
