#ifndef RAW_SOCKET_H_
#define RAW_SOCKET_H_

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
#include "../NetworkLayer/mipd.h"

#define BROADCAST_ADDRESS {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define ETH_P_MIP 0x88B5
#define MAX_EVENTS	10
#define MAX_IF		3
#define MAX_INTERFACES 10
#define PACKET_MAX_SIZE 100

struct ether_frame {
	uint8_t dst_addr[6];
	uint8_t src_addr[6];
	uint8_t eth_proto[2];
	uint8_t contents[0];
} __attribute__((packed));

struct ifs_data {
    struct sockaddr_ll addr[MAX_INTERFACES];
    int rsock[MAX_INTERFACES]; // Socket for hvert grensesnitt
    int ifn;  // Antall grensesnitt
};


/* creates and returns a raw socket */
int setupRawSocket();

struct mip_pdu * recv_pdu_from_raw(int rawSocket, uint8_t *src_mac_addr);
/*here we loop through all the interfaces looking for an interface with*/
void print_mac_addr(uint8_t *addr, size_t len);
int send_broadcast_message(int raw, struct ifs_data *ifs, struct mip_pdu* pdu );

void get_mac_from_interfaces(struct ifs_data *);

void init_ifs(struct ifs_data *ifs);

int send_raw_packet(int raw_socket, struct mip_pdu *mip_pdu, uint8_t *dst_mac_address, struct sockaddr_ll *addr);
int send_arp_response(int rawSocket,  struct mip_pdu *received_pdu, size_t length, uint8_t *dst_mac_addr, uint8_t *self_mip_addr, struct sockaddr_ll *addr);

#endif