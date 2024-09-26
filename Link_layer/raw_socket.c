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

#define ETH_P_MIP 0x88B5
int status;

struct ether_frame {
	uint8_t dst_addr[6];
	uint8_t src_addr[6];
	uint8_t eth_proto[2];
	uint8_t contents[0];
} __attribute__((packed));

/* creates and returns a raw socket */
int setupRawSocket(){
    int status;
    int raw_sockfd=(AF_PACKET , SOCK_RAW, htons(ETH_P_MIP)); //all protocols
    if(raw_sockfd == -1){
        fprintf(stderr, "Error: could not create raw socket ");
        close(raw_sockfd);
        exit(EXIT_FAILURE);
    }

    return raw_sockfd;
}

int sendRawPacket(int rawSocket){






	
}

int recvRawPacket(int rawSocket){

}
