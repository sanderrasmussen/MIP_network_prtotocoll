#ifndef MIPD_ARP_H_   /* Include guard */
#define MIPD_ARP_H_
#define SDU_LENGTH 
#include <stdint.h>


/* In total this mip arp message will be 32 bit*/
struct mip_arp_message{
    uint32_t type : 1;
    uint32_t address : 8;
    uint32_t padding : 23;
}__attribute__((packed));
/* mip arp message types */
#define REQUEST 0x00
#define RESPONSE 0x01


//socket related stuff


#endif // MIP_H_
