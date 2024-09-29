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

#define DST_MAC_ADDR {0x00, 0x00, 0x00, 0x00, 0x00, 0x02}

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

int send_raw_packet(int rawSocket, struct sockaddr_ll *socket_name, uint8_t *payload, size_t length){
    int status;
    struct ether_frame ether_frame_header;
    /* msgheader is what we send over link layer*/
    struct msghdr *message;
    /*the input output vector[0] will point tp the ether frame and the vector[1] will point to the payload*/
    struct iovec msgvec[2];

    // fill out the ether frame
    uint8_t destinations_mac_address[]= DST_MAC_ADDR;
    memcpy(ether_frame_header.dst_addr, destinations_mac_address, 6);
    memcpy(ether_frame_header.src_addr, socket_name->sll_addr, 6);
    ether_frame_header.eth_proto[0] =ETH_P_MIP; //using the mip protocoll

    /* fill out iovec with ethrenet frame and payload*/
    //frame
    msgvec[0].iov_base= &ether_frame_header;
    msgvec[0].iov_len = sizeof(struct ether_frame);
    //payload
    msgvec[1].iov_base = payload;
    msgvec[1].iov_len = length;

    message = (struct msghdr *) calloc(1, sizeof(struct msghdr));
    //filling out rest of messageheader data
    message->msg_name = socket_name;
    message->msg_namelen = sizeof(struct sockaddr_ll);
    message->msg_iov = msgvec;
    message->msg_iovlen =2;//only ether frame and payload

    printf("sending message from dst address: %d",socket_name->sll_addr);
    /* now everything should be filled out and we can send the message, which contains everything above*/
    status= sendmsg(rawSocket, message, 0);
    if (status==-1){
        free(message);
        perror("failed at sending raw packet");
        exit(1);
    }

    free(message);
    return 1;
}
int recv_raw_packet(int rawSocket, uint8_t *buffer, size_t length){
    int status;
    struct sockaddr_ll socket_name;
    struct ether_frame ethernet_frame_header;
    struct msghdr message;
    struct iovec msgvec[2];

    //setting up structure for saving incomming packets into our own structs
    msgvec[0].iov_base=&ethernet_frame_header;
    msgvec[0].iov_len = sizeof(struct ether_frame );
    msgvec[1].iov_base = buffer;
    msgvec[1].iov_len = length;

    message.msg_name= &socket_name;
    message.msg_namelen= sizeof(struct sockaddr_ll);
    message.msg_iovlen = 2;
    message.msg_iov = msgvec;

    status = recvmsg(rawSocket, &message,0 );
    if (status==-1){
        perror("could not receive raw socket message");
        exit(1); 
    }

    print_mac_addr(ethernet_frame_header.src_addr, 6);
    return status;
}

/*here we loop through all the interfaces looking for an interface with*/
int get_mac_from_interface(struct sockaddr_ll *socket_name){
    struct ifaddrs *interfaces, *interfaceIterator;
    /* Store interfaces in linkedlist with head being interfaces pointer*/
    if(getifaddrs(&interfaces)){
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    /* iterate throug the linkedList*/
    for (interfaceIterator = interfaces; interfaceIterator!=NULL; interfaceIterator->ifa_next){

        /*checking that the interface address is not null*/
        if(interfaceIterator->ifa_addr != NULL && interfaceIterator->ifa_addr->sa_family==AF_PACKET && (strcmp("lo",interfaceIterator->ifa_name))){
            /* now we copy the mac address into the interfaceIterator*/
            memcpy(socket_name, (struct sockaddr_ll*)interfaceIterator->ifa_addr, sizeof(struct sockaddr_ll));
        }
        /* now we hopefully have the mack addres stored in socket_name*/
    }
    freeifaddrs(interfaces);

    return 1;
}
void print_mac_address(uint8_t *addr, size_t len)
{
	int i;

	for (i = 0; i < len - 1; i++) {
		printf("%d:", addr[i]);
	}
	printf("%d", addr[i]);
}
