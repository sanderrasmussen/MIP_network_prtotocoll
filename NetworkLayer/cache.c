#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/stat.h>
#include "mipd.h"
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
#include "cache.h"

//to be implemented.
struct entry* get_mac_from_cache(struct cache *cache, uint8_t mip_address){
    struct entry *current= cache->head;

    while(current!=NULL){
        if (current->mip_address==mip_address){
            return current;
        }
        current= current->next;
    }
    return NULL;
}
// to be implemented
int add_to_cache(struct cache *cache, uint8_t mip_address, uint8_t mac_address ){
    struct entry *newEntry= malloc(sizeof(struct entry));
    if (newEntry==NULL){
        perror("Allocate cache entry memory in add to cache function");
        return -1;
    }
    newEntry->mip_address= mip_address;
    memcpy(newEntry->mac_address, mac_address, sizeof(newEntry->mac_address));

    if (cache->head==NULL){
        cache->head = newEntry;
        cache->tail = newEntry;
    }
    else{
        cache->tail->next = newEntry;
        cache->tail = newEntry;
    }

    return 1;
}