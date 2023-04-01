all: master slave

master: master.c
	gcc -Wall -std=c99 $< -o $@.out

slave: slave.c
	gcc -Wall -std=c99 $< -o $@.out


clean:
	rm -f master.out slave.out

.PHONY: all clean
