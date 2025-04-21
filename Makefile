CC = gcc

all:
	$(CC) -c json.c
	$(CC) -c jsonrpc.c
	$(CC) json_demo.c json.o -lm -o json_demo

clean:
	rm *.o
	-rm json_demo
