#ifndef CACHE_H_   /* Include guard */
#define CACHE_H_
#include <stdint.h>

struct cache{
    struct entry *head;
    struct entry *tail;
};
//super simple linked list for dynamycly storing cache entries
struct entry{
    uint8_t mip_address;
    uint8_t mac_address[6];
    struct entry *next;
};
//returns -1 if no entry is found. else it returns the mac adddress
int get_mac_from_cache(uint8_t mip_address, uint8_t *dst_addr_buffer);
int add_to_cache(uint8_t mip_address, uint8_t mac_address );
#endif // MIP_H_