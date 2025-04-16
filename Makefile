CC = gcc

all:
	$(CC) -c json.c
	$(CC) json_demo.c json.o -lm -o json_demo
	#$(CC) json_demo_adv.c json.o -lm -o json_demo_adv

clean:
	rm *.o
	-rm json_demo
	#-rm json_demo_adv

