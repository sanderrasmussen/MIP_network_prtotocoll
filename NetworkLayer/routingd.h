#ifndef ROUTINGD_H_   /* Include guard */
#define ROUTINGD_H_

#include <stdint.h>
#define R 0x52
#define E 0x45
#define Q 0x51
#define S 0x53
#define P 0x50

struct RoutingRequest {
    uint8_t src_mip_addr;      // MIP address of the host sending the request
    uint8_t ttl;               // Time-to-Live (0 in this case)
    uint8_t r;                 // 'R' (0x52)
    uint8_t e;                 // 'E' (0x45)
    uint8_t q;                 // 'Q' (0x51)
    uint8_t dest_mip_addr;     // MIP address to look up
};

struct RoutingResponse {
    uint8_t src_mip_addr;      // MIP address of the responding host
    uint8_t ttl;               // Time-to-Live (0 in this case)
    uint8_t r;                 // 'R' (0x52)
    uint8_t s;                 // 'S' (0x53)
    uint8_t p;                 // 'P' (0x50)
    uint8_t next_hop_mip_addr; // Next hop MIP address (255 if no route)
};


struct RoutingResponse create_routing_response(uint8_t src_addr, uint8_t next_hop_addr);
struct RoutingRequest create_routing_request(uint8_t src_addr, uint8_t dest_addr) ;

#endif // MIP_H_