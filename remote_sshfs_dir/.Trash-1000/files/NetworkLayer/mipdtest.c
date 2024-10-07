#define ETH_P_MIP 0x88B5
#define MIP_ADDRESS 255

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


int main(int argc, char *argv[])
{
	int	raw_sock, rc;
    char buffer[100];
	struct epoll_event ev, events[10];
	int epollfd;

	/* Set up a raw AF_PACKET socket without ethertype filtering */

    short unsigned int mip = 0x88B5;
    int raw_sockfd=socket(AF_PACKET , SOCK_RAW, htons(mip));
    if(raw_sock == -1){
        perror( "Error: could not create raw socket ");
        exit(EXIT_FAILURE);
    }
 

    struct sockaddr_ll *socket_name = malloc(sizeof(struct sockaddr_ll));
	/* Walk through all interfaces of the node and store their addresses */
    get_mac_from_interface(socket_name);

	/* Set up epoll */
	epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epoll_create1");
		close(raw_sock);
		exit(EXIT_FAILURE);
	}

	/* Add RAW socket to epoll */
	ev.events = EPOLLIN|EPOLLHUP;
	ev.data.fd = raw_sock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, raw_sock, &ev) == -1) {
		perror("epoll_ctl: raw_sock");
		close(raw_sock);
		exit(EXIT_FAILURE);
	}

	/* epoll_wait forever for incoming packets */
	while(1) {
		rc = epoll_wait(epollfd, events,10, -1);
		if (rc == -1) {
			perror("epoll_wait");
			break;
		} else if (events->data.fd == raw_sock) {
			printf("\n<info> The neighbor is initiating a handshake\n");
			rc = recv_raw_packet(raw_sock,buffer,100 );
			if (rc < 1) {
				perror("recv");
				break;
			}
		}
	}

	close(raw_sock);
	return 0;
}
