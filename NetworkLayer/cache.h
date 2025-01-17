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
    struct pdu_node *pdu_list; //for simplification, we will only hold one pdu in queue, This will always be a PING packet 
    struct sockaddr_ll *if_addr;
};
struct pdu_node {
    struct mip_pdu *pdu;      // pointer to the PDU
    struct pdu_node *next;    // pointer to the next PDU in the list
};



//returns -1 if no entry is found. else it returns the mac adddress
struct entry* get_mac_from_cache(struct cache *cache, uint8_t mip_address);
int add_to_cache(struct cache *cache, uint8_t mip_address, uint8_t *mac_address, struct sockaddr_ll *if_addr );
int add_pdu_to_queue(struct cache * cache ,uint8_t mip_address , struct mip_pdu* pdu);
struct mip_pdu *fetch_queued_pdu_in_cache(struct cache *cache , uint8_t mip_address);
int destroy_cache(); //to be implemented
void clear_cache(struct cache *cache);
#endif // MIP_H_
