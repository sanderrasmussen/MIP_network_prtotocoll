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


// Function for getting mac from cache belonging to a given MIP address
struct entry* get_mac_from_cache(struct cache *cache, uint8_t mip_address){
    struct entry *current = cache->head;

    // search 
    while(current != NULL){
        if (current->mip_address == mip_address){
            return current;  // Returner oppføringen hvis funnet
        }
        current = current->next;
    }
    return NULL;  // return if not found 
}
// Function to add or update an entry in the cache
int add_to_cache(struct cache *cache, uint8_t mip_address, uint8_t *mac_address, struct sockaddr_ll *if_addr) {
    struct entry *current = cache->head;

    // Check if the entry already exists in the cache
    while (current != NULL) {
        if (current->mip_address == mip_address) {
            // Update existing entry
            if (mac_address != NULL) {
                memcpy(current->mac_address, mac_address, sizeof(current->mac_address));
            }
            if (if_addr != NULL) {
                current->if_addr = if_addr;
            }
            return 1;  // Updated entry successfully
        }
        current = current->next;
    }

    // If entry doesn't exist, create a new one
    struct entry *new_entry = malloc(sizeof(struct entry));
    if (new_entry == NULL) {
        perror("Unable to allocate memory for new cache entry");
        return -1;
    }

    // Initialize the new entry
    new_entry->mip_address = mip_address;
    if (mac_address != NULL) {
        memcpy(new_entry->mac_address, mac_address, sizeof(new_entry->mac_address));
    }
    new_entry->if_addr = if_addr;
    new_entry->pdu_list = NULL;  // Initialize the PDU list as empty
    new_entry->next = NULL;

    // Add the new entry to the cache
    if (cache->head == NULL) {
        cache->head = new_entry;
        cache->tail = new_entry;
    } else {
        cache->tail->next = new_entry;
        cache->tail = new_entry;
    }

    return 1;  // Return success
}


// Function to add a PDU to the queue linked list for a specific MIP address
int add_pdu_to_queue(struct cache *cache, uint8_t mip_address, struct mip_pdu *pdu) {
    struct entry *current = cache->head;

    // traverse the cache to find the entry with the matching MIP address
    while (current != NULL) {
        if (current->mip_address == mip_address) {
            // Create a new node for the PDU
            struct pdu_node *new_node = malloc(sizeof(struct pdu_node));
            if (new_node == NULL) {
                perror("Unable to allocate memory for new PDU node");
                return -1;
            }
            new_node->pdu = pdu;
            new_node->next = NULL;

            // Add the new PDU node to the end of the linked list
            if (current->pdu_list == NULL) {
                current->pdu_list = new_node;
            } else {
                struct pdu_node *last = current->pdu_list;
                while (last->next != NULL) {
                    last = last->next;
                }
                last->next = new_node;
            }

            printf("PDU added to queue for mip_address: %d\n", mip_address);
            return 1;  //  success
        }
        current = current->next;
    }

    // If the MIP address was not found in the cache, return -1
    printf("Could not find entry for mip_address: %d\n", mip_address);
    return -1;
}

// Function to fetch and remove the first unsent PDU from the cache
struct mip_pdu *fetch_queued_pdu_in_cache(struct cache *cache, uint8_t mip_address) {
    struct entry *current = cache->head;

    // Traverse the cache to find the entry with the matching MIP address
    while (current != NULL) {
        if (current->mip_address == mip_address && current->pdu_list != NULL) {
            // Retrieve the first PDU in the list
            struct pdu_node *first_node = current->pdu_list;
            struct mip_pdu *pdu = first_node->pdu;

            // Remove the first node from the list
            current->pdu_list = first_node->next;
            free(first_node);  // Free the memory of the node

            return pdu;  // Return the PDU
        }
        current = current->next;
    }
    printf("--- No unsent pdu found in cache\n");
    return NULL;  // Return NULL if no unsent PDU exists
}
// Function to clear the cache and free memory
void clear_cache(struct cache *cache) {
    struct entry *current = cache->head;

    // Traverse and free the cache
    while (current != NULL) {
        struct entry *next_entry = current->next;

        // Free the linked list of PDUs
        struct pdu_node *pdu_node = current->pdu_list;
        while (pdu_node != NULL) {
            struct pdu_node *next_pdu = pdu_node->next;
            free(pdu_node->pdu); 
            free(pdu_node);        
            pdu_node = next_pdu;
        }

        free(current);  // the cache entry
        current = next_entry;
    }
    cache->head = NULL;
    cache->tail = NULL;
}
