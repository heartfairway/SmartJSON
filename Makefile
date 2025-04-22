CC = gcc

all:
	$(CC) -c json.c
	$(CC) -c jsonrpc.c
	$(CC) json_demo.c json.o -lm -o json_demo
	$(CC) jsonrpc_demo.c json.o jsonrpc.o -lm -o jsonrpc_demo

clean:
	rm *.o
	-rm json_demo
	-rm jsonrpc_demo
