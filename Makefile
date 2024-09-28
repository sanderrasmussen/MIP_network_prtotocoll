CC 		= gcc
CFLAGS 	= -g
RM 		= rm -f

default: all

all: mipd ping_client

mipd: mipd.o unix_socket.o
	$(CC) $(CFLAGS) -o mipd NetworkLayer/mipd.o Application_layer/unix_socket.o

mipd.o: NetworkLayer/mipd.c
	$(CC) $(CFLAGS) -c NetworkLayer/mipd.c -o NetworkLayer/mipd.o

unix_socket.o: Application_layer/unix_socket.c
	$(CC) $(CFLAGS) -c Application_layer/unix_socket.c -o Application_layer/unix_socket.o

ping_client: ping_client.o unix_socket.o
	$(CC) $(CFLAGS) -o ping_client ping_client.o Application_layer/unix_socket.o

ping_client.o: ping_client.c
	$(CC) $(CFLAGS) -c ping_client.c -o ping_client.o

clean:
	$(RM) NetworkLayer/mipd.o Application_layer/unix_socket.o mipd
	$(RM) ping_client.o Application_layer/unix_socket.o ping_client


veryclean: clean

.PHONY: clean veryclean all default
