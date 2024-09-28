
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>	/* free */
#include <stdio.h>	/* printf */
#include <unistd.h>	/* fgets */
#include <string.h>	/* memset */
#include <sys/socket.h>	/* socket */
#include <fcntl.h>
#include <sys/epoll.h>		/* epoll */
#include <linux/if_packet.h>	/* AF_PACKET */
#include <net/ethernet.h>	/* ETH_* */
#include <arpa/inet.h>	/* htons */
#include <ifaddrs.h>	/* getifaddrs */
#include "Application_layer/unix_socket.c"

int main(int argc, char *argv[]){

    struct socketaddr_un *address;
    char *pathToSocket = "/tmp/unix.sock";
    
    //testing that unix socket is porperly set up
    int unix_connection_socket = setupUnixSocket(pathToSocket, address);
    int unix_data_socket =unixSocket_connect(unix_connection_socket,pathToSocket);
    char *payload = "hello world";
    unixSocket_send(payload, unix_data_socket);
    close(unix_connection_socket);
    close(unix_data_socket);
    unlink(pathToSocket);
    exit(0);

}

