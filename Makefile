CC = gcc

all:
	$(CC) -c -fPIC json.c jsonrpc.c
	$(CC) -shared -o libjson.so json.o jsonrpc.o
	ar rcs libjson.a json.o jsonrpc.o

	$(CC) -o json_demo json_demo.c libjson.a
	$(CC) -o jsonrpc_demo jsonrpc_demo.c libjson.a

clean:
	rm *.o *.a *.so 
	-rm json_demo
	-rm jsonrpc_demo
