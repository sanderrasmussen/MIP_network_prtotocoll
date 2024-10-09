#ifndef MIPD_H_   /* Include guard */
#define MIPD_H_

#include <stdint.h>



/* In total this structure should only take 32 bits, hopefully*/
typedef struct mip_header{
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

typedef struct mip_pdu{
    struct mip_header mip_header;
    uint32_t sdu; //this is the payload, must be 32, divisible by 4. padding can be implemented in an upper layer protocol
}__attribute__((packed));

/* In total this mip arp message will be 32 bit*/
typedef struct mip_arp_message{
    uint32_t type : 1;
    uint32_t address : 8;
    uint32_t padding : 23;
}__attribute__((packed));
/* mip arp message types */
#define REQUEST 0x00
#define RESPONSE 0x01


//socket related stuff


#endif // MIP_H_
