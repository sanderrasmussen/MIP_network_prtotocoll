CC 		= gcc
CFLAGS 	= -g
RM 		= rm -f

default: all

all: mipd

mipd: mipd.o unix_socket.o
	$(CC) $(CFLAGS) -o mipd NetworkLayer/mipd.o Application_layer/unix_socket.o

mipd.o: NetworkLayer/mipd.c
	$(CC) $(CFLAGS) -c NetworkLayer/mipd.c -o NetworkLayer/mipd.o

unix_socket.o: Application_layer/unix_socket.c
	$(CC) $(CFLAGS) -c Application_layer/unix_socket.c -o Application_layer/unix_socket.o

clean:
	$(RM) NetworkLayer/mipd.o Application_layer/unix_socket.o mipd

veryclean: clean

.PHONY: clean veryclean all default
