#include <stdint.h>
#include <stdlib.h>	/* free, malloc, exit */
#include <stdio.h>	/* printf, perror */
#include <unistd.h>	/* close, ssize_t */
#include <string.h>	/* memset, memcpy, strlen */
#include <sys/socket.h>	/* socket, bind, struct msghdr */
#include <fcntl.h>	/* file control options */
#include <sys/ioctl.h> /* ioctl */
#include <sys/epoll.h>	/* epoll */
#include <linux/if_packet.h>	/* AF_PACKET, struct sockaddr_ll */
#include <net/ethernet.h>	/* ETH_P_MIP, ETH_P_ALL */
#include <arpa/inet.h>	/* htons */
#include <ifaddrs.h>	/* getifaddrs, struct ifaddrs */
#include <net/if.h>   /* struct ifreq, SIOCGIFINDEX */
#include <sys/ioctl.h>
#include <net/if.h>
#include "raw_socket.h" /* Relevant interfaces from raw_socket.h */
#include "mip_arp.h" /* Relevant interfaces from mip_arp.h */
#include "../NetworkLayer/mipd.h" /* Relevant interfaces from mipd.h */

#define ETH_P_MIP 0x88B5

void print_mac_addr(uint8_t *addr, size_t len)
{
	size_t i;

	for (i = 0; i < len - 1; i++) {
		printf("%02x:", addr[i]);
	}
	printf("%02x\n", addr[i]);
}

void get_mac_from_interfaces(struct ifs_data *ifs)
{
	struct ifaddrs *ifaces, *ifp;
	int i = 0;

	/* Enumerate interfaces: */
	/* Note in man getifaddrs that this function dynamically allocates
	   memory. It becomes our responsability to free it! */
	if (getifaddrs(&ifaces)) {
		perror("getifaddrs");
		exit(-1);
	}

	/* Walk the list looking for ifaces interesting to us */
	for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next) {
		/* We make sure that the ifa_addr member is actually set: */
		if (ifp->ifa_addr != NULL &&
		    ifp->ifa_addr->sa_family == AF_PACKET &&
		    strcmp("lo", ifp->ifa_name))
		/* Copy the address info into the array of our struct */
		memcpy(&(ifs->addr[i++]),
		       (struct sockaddr_ll*)ifp->ifa_addr,
		       sizeof(struct sockaddr_ll));
	}
	/* After the for loop, the address info of all interfaces are stored */
	/* Update the counter of the interfaces */
	ifs->ifn = i;

	/* Free the interface list */
	freeifaddrs(ifaces);
}
void bind_socket_to_interface(int sockfd, const char *interface, int protocol) {
    struct sockaddr_ll sll;
    struct ifreq ifr;

    // Kopier interface-navnet til ifreq-strukturen
    strncpy(ifr.ifr_name, interface, IFNAMSIZ);

    // Hent grensesnittets indeks
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1) {
        perror("Unable to get interface index");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Konfigurer sockaddr_ll for å binde til riktig interface
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(protocol);

    // Bind socket til grensesnittet
    if (bind(sockfd, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
        perror("Socket binding failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Socket bound to interface %s\n", interface);
}

void init_ifs(struct ifs_data *ifs) {
    struct ifaddrs *ifaces, *ifp;
    int i = 0;

    if (getifaddrs(&ifaces)) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    // Opprett en socket per grensesnitt
    for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next) {
        if (ifp->ifa_addr != NULL &&
            ifp->ifa_addr->sa_family == AF_PACKET &&
            strcmp("lo", ifp->ifa_name)) {

            // Opprett socket for hvert grensesnitt
            int sockfd = setupRawSocket();
            bind_socket_to_interface(sockfd, ifp->ifa_name, ETH_P_MIP);  // Bind socket til grensesnitt

            // Lagre socket og adresseinformasjon i ifs_data
            ifs->rsock[i] = sockfd;
            memcpy(&(ifs->addr[i]), (struct sockaddr_ll *)ifp->ifa_addr, sizeof(struct sockaddr_ll));
            i++;
        }
    }
    ifs->ifn = i;  // Antall grensesnitt
    freeifaddrs(ifaces);
}


/* creates and returns a raw socket */
int setupRawSocket(){
    short unsigned int mip = 0x88B5;
    int raw_sockfd=socket(AF_PACKET , SOCK_RAW, htons(ETH_P_MIP)); //all protocols
    if(raw_sockfd == -1){
        fprintf(stderr, "Error: could not create raw socket ");
        exit(EXIT_FAILURE);
    }

    return raw_sockfd;

}
struct mip_pdu * recv_pdu_from_raw(int rawSocket, uint8_t *src_mac_addre){ // the src mac addr is the buffer we store the mac address of the sender in so that we can cache the mip and mac
    int status;
    size_t pdu_length = sizeof(uint64_t)+ 100; //hardcoded sadly
    struct mip_pdu *pdu = malloc(sizeof(pdu_length));
    struct sockaddr_ll socket_name;
    struct ether_frame ethernet_frame_header;
    struct msghdr message;
    struct iovec msgvec[2];
    char *serilzd_pdu = malloc(pdu_length);

    // Nullstill strukturen for å unngå uønskede verdier
    memset(&socket_name, 0, sizeof(struct sockaddr_ll));
    memset(&ethernet_frame_header, 0, sizeof(struct ether_frame));

    // Setting up structure for saving incoming packets into our own structs
    msgvec[0].iov_base = &ethernet_frame_header;
    msgvec[0].iov_len = sizeof(struct ether_frame);
    msgvec[1].iov_base = serilzd_pdu;
    msgvec[1].iov_len = pdu_length;

    message.msg_name = &socket_name;
    message.msg_namelen = sizeof(struct sockaddr_ll);
    message.msg_iov = msgvec;
    message.msg_iovlen = 2;

    // Receive the raw packet
    status = recvmsg(rawSocket, &message, 0);
    if (status == -1) {
        perror("Could not receive raw socket message");
        return -1;  // Avoid exiting, return error code instead
    }

    //store sender mac in buffer
    memcpy(src_mac_addre, ethernet_frame_header.src_addr,6);
    //derserialize pdu buffer
    pdu = deserialize_pdu(serilzd_pdu, pdu_length);

    printf(" received packet from src mip addresse  : %d \n ",  pdu->mip_header.src_addr);

    if(pdu->mip_header.sdu_type==MIP_ARP){
        printf(" mip Arp :who has : %d \n ", pdu->sdu.arp_msg_payload->address);
    }
    else{
    printf(" message : %s \n ", pdu->sdu.message_payload);
    }
    
    return pdu;
}

int send_raw_packet(int raw_socket, struct mip_pdu *mip_pdu, uint8_t *dst_mac_address, struct sockaddr_ll *addr) {
    struct ether_frame ethernet_header;
    struct msghdr *message_header;
    struct iovec ioVector[2];
    int status;

    // Serialiser PDU-en
    char *serilzd_pdu = malloc((sizeof(uint32_t) * 2) + 100); // Temp hardkodet buffer
    serialize_pdu(mip_pdu, serilzd_pdu);

    // Fyll ut Ethernet-overskriften
    memcpy(ethernet_header.dst_addr, dst_mac_address, 6);  // Bruk kringkastingsadressen
    memcpy(ethernet_header.src_addr, addr->sll_addr, 6);   // Bruk riktig kilde-MAC-adresse fra ifs_data

    // Tilordning av riktig protokollverdi for MIP (ETH_P_MIP)
    ethernet_header.eth_proto[0] = (ETH_P_MIP >> 8) & 0xFF;  // Høybyte
    ethernet_header.eth_proto[1] = ETH_P_MIP & 0xFF;         // Lavbyte

    // Klargjør melding
    ioVector[0].iov_base = &ethernet_header;
    ioVector[0].iov_len = sizeof(struct ether_frame);
    ioVector[1].iov_base = serilzd_pdu;
    ioVector[1].iov_len = sizeof(mip_pdu) + ARP_SDU_SIZE + SDU_MESSAGE_MAX_SIZE;

    message_header = (struct msghdr *)calloc(1, sizeof(struct msghdr));
    message_header->msg_name = addr;  // Bruk riktig socket-adresse for valgt grensesnitt
    message_header->msg_namelen = sizeof(struct sockaddr_ll);
    message_header->msg_iov = ioVector;
    message_header->msg_iovlen = 2;

    // Send meldingen via riktig socket
    status = sendmsg(raw_socket, message_header, 0);
    if (status == -1) {
        perror("Error sending raw packet");
        free(message_header);
        exit(EXIT_FAILURE);
    }

    printf("Raw packet sent to: ");
    print_mac_addr(ethernet_header.dst_addr, 6);  // Skriver ut MAC-adressen pakken ble sendt til

    free(mip_pdu);
    free(message_header);
    return 1;
}

int send_arp_response(int rawSocket,  struct mip_pdu *received_pdu, size_t length, uint8_t *dst_mac_addr, uint8_t *self_mip_addr, struct sockaddr_ll *addr){
    /* create mip pdu first of type ARP RESPONSE , the message argument does not matter here*/
    received_pdu->sdu.arp_msg_payload->type= RESPONSE;
    uint8_t sender_mip_addr = received_pdu->mip_header.src_addr;
    received_pdu->mip_header.dest_addr = sender_mip_addr;
    received_pdu->mip_header.src_addr= self_mip_addr;
    if(received_pdu==NULL){
        perror("making response arp pdu ");
        return -1;
    }

    printf("sending arp response to :");
    print_mac_addr(dst_mac_addr,6);
    
    printf("\n arp packet addr %d \n", received_pdu->sdu.arp_msg_payload->address);
    send_raw_packet(rawSocket, received_pdu, dst_mac_addr, addr);
    printf(" ARP RESPONSE packet sent to  ");
    print_mac_addr(dst_mac_addr, 6);
    printf('\n');
    return 1;
}

int handle_arp_packet(int raw_sock ,struct mip_pdu *arp_message ){

    //if we reached this, then the packet is not an arp message
    close(raw_sock);
    return 1;
}


//THESE PDU METHODS SHOULD BE MOVED TO SEPERATE FILE FOR ORGANIZINGS SAKE

