#ifndef MIPD_H_   /* Include guard */
#define MIPD_H_
#define SDU_LENGTH 
#include <stdint.h>
#include "../Application_layer/unix_socket.h"
#include "../Link_layer/mip_arp.h"
#define MIP_HEADER_SIZE 4
#define ARP_SDU_SIZE 8
#define SDU_MESSAGE_MAX_SIZE 100

/* In total this structure should only take 32 bits, hopefully*/
struct mip_header{
    uint8_t dest_addr : 8;
    uint8_t src_addr : 8;
    uint8_t ttl : 4; // recomended to be set to value of 1
    uint16_t sdu_len : 9;
    uint8_t sdu_type : 3;   
}__attribute__((packed));
/* this is relevant for the sdu_type,
which can be mip_arp og ping*/
#define MIP_ARP 0x01
#define PING 0x02
#define ROUTER 0x04

//for the router the mip pdu will contain the routing table information
struct mip_pdu{
    struct mip_header mip_header; //32 bits
    union{
        struct mip_arp_message *arp_msg_payload; //32 bits
        //we have an add padding function that will add the needed padding
        char *message_payload;//this is the payload, must be 32, divisible by 4. padding can be implemented in an upper layer protocol
    }sdu;
}__attribute__((packed));

//struct mip_pdu* create_mip_pdu( uint8_t sdu_type, uint8_t arp_type, uint8_t dst_mip_addr, char *message, uint8_t src_address);
size_t serialize_pdu(struct mip_pdu *mip_pdu, char* buffer);
struct mip_pdu* deserialize_pdu(char* buffer, size_t length);

#endif // MIP_H_
