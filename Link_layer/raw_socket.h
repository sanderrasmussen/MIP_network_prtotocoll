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

struct ether_frame {
	uint8_t dst_addr[6];
	uint8_t src_addr[6];
	uint8_t eth_proto[2];
	uint8_t contents[0];
} __attribute__((packed));

/* creates and returns a raw socket */
int setupRawSocket();
int send_raw_packet(int rawSocket, struct sockaddr_ll *socket_name, char *payload, size_t length, uint8_t dst_mac_addr[6]);
int recv_raw_packet(int rawSocket, struct mip_pdu *buffer, size_t length);
/*here we loop through all the interfaces looking for an interface with*/
int get_mac_from_interface(struct sockaddr_ll *socket_name);
void print_mac_address(uint8_t *addr, size_t len);
int raw_socket_listen();
int send_arp(int raw_socket, struct sockaddr_ll *socket_name, struct mip_pdu *mip_pdu);
#endif