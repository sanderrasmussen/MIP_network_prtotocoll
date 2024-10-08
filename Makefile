CC      = gcc
CFLAGS  = -g -Wall -Wextra
RM      = rm -f

default: all

all: mipd ping_client 

mipd: mipd.o unix_socket.o raw_socket.o cache.o
	$(CC) $(CFLAGS) -o mipd NetworkLayer/mipd.o Application_layer/unix_socket.o Link_layer/raw_socket.o NetworkLayer/cache.o

mipd.o: NetworkLayer/mipd.c
	$(CC) $(CFLAGS) -c NetworkLayer/mipd.c -o NetworkLayer/mipd.o

unix_socket.o: Application_layer/unix_socket.c
	$(CC) $(CFLAGS) -c Application_layer/unix_socket.c -o Application_layer/unix_socket.o

raw_socket.o: Link_layer/raw_socket.c
	$(CC) $(CFLAGS) -c Link_layer/raw_socket.c -o Link_layer/raw_socket.o
	
ping_client: ping_client.o unix_socket.o
	$(CC) $(CFLAGS) -o ping_client ping_client.o Application_layer/unix_socket.o

ping_client.o: ping_client.c
	$(CC) $(CFLAGS) -c ping_client.c -o ping_client.o

cache.o: NetworkLayer/cache.c
	$(CC) $(CFLAGS) -c NetworkLayer/cache.c -o NetworkLayer/cache.o


clean:
	$(RM) NetworkLayer/mipd.o Application_layer/unix_socket.o Link_layer/raw_socket.o mipd
	$(RM) ping_client.o ping_client

	

