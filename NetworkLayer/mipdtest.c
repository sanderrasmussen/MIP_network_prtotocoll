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
    int rc;
    unsigned char buffer[100];
	struct epoll_event ev, events[10];


	/* Set up a raw AF_PACKET socket without ethertype filtering */

    short unsigned int mip = 0x88B5;
    int raw_sock=setupRawSocket();
 

    struct sockaddr_ll *socket_name = malloc(sizeof(struct sockaddr_ll));
	/* Walk through all interfaces of the node and store their addresses */
    get_mac_from_interface(socket_name);

    
	

	close(raw_sock);
	return 0;
}
