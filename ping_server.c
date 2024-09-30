
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
#include "Link_layer/raw_socket.h"
#define MIP_ADDRESS 2
#define MAX_EVENTS 10   

int main(int argc, char *argv[]){

    int status;
    /* testing raw socket, making sure it works*/

    struct sockaddr_ll socket_name;
    /* Setting up epollin variables*/
    struct epoll_event event, events[MAX_EVENTS];
    int epoll_fd;

    char buffer[1024];


    int raw_socket = setupRawSocket();
    if (raw_socket == -1){
        perror("raw socket fail");
        exit(EXIT_FAILURE);
    }

    //listen for connections
    status = recv_raw_packet(raw_socket, buffer, 1024);








    exit(0);

}

