
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
#include "Application_layer/unix_socket.h"

int main(int argc, char *argv[]){

    struct sockaddr_un *address= malloc(sizeof(struct sockaddr_un));
    char *pathToSocket = "/tmp/unix.sock";
    
    //testing that unix socket is porperly set up
    int unix_data_socket = setupUnixSocket(pathToSocket, address);
 
    unixSocket_connect(unix_data_socket, pathToSocket, address);
    char *payload = "hello world";
    unixSocket_send(unix_data_socket, payload);
    printf("message sendt");
    close(unix_data_socket);

    exit(0);

}

