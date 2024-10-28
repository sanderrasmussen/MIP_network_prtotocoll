// TODO: set up unix socket with epoll to listen for mipd connection
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/stat.h>
#include "mipd.h"
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
#include "../Application_layer/unix_socket.h"
#include "../Link_layer/raw_socket.h"
#include "cache.h"

int handle_router_events(){
    
}

int main(int argc, char *argv[]) {

    if (argc != 2 || strcmp(argv[1], "-h") == 0) {
        printf("Usage: mipd [-h] [-d] <socket_upper> <MIP address>");
        exit(EXIT_FAILURE);
    }
    printf("server started \n");
    
    char *socket_path = sizeof(argv[1]) + sizeof("_routingd");
    strcpy(socket_path, argv[1]);
    strcat(socket_path, "_routingd");//read online that only one can bind to a unix socket at a time, therefore need a seperate unix socket from the mipd and server so that they can all be in listening mode using epoll
    struct sockaddr_un address;

    int mask = umask(0);
    unlink(socket_path);
    int unix_socket = setupUnixSocket(socket_path, &address);
    unixSocket_bind(unix_socket, socket_path, &address);
    unixSocket_listen(unix_socket, socket_path, unix_socket);
    umask(mask);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = unix_socket;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, unix_socket, &event) == -1) {
        perror("epoll_ctl");
        close(unix_socket);
        exit(EXIT_FAILURE);
    }

    handle_router_events(epoll_fd, unix_socket);
    close(unix_socket);

    return 0;
}
