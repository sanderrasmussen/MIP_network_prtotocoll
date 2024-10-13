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
// Funksjon for å hente en oppføring fra cachen basert på MIP-adresse
struct entry* get_mac_from_cache(struct cache *cache, uint8_t mip_address){
    struct entry *current = cache->head;

    // Gå gjennom cache-listen for å finne matchende MIP-adresse
    while(current != NULL){
        if (current->mip_address == mip_address){
            return current;  // Returner oppføringen hvis funnet
        }
        current = current->next;
    }
    return NULL;  // Returner NULL hvis ikke funnet
}

// Funksjon for å legge til eller oppdatere MAC-adresse i cachen
int add_to_cache(struct cache *cache, uint8_t mip_address, uint8_t *mac_address, struct sockaddr_ll *if_addr){
    
    // Først, se om oppføringen allerede finnes i cachen
    struct entry *current = cache->head;
    while(current != NULL){
        if (current->mip_address == mip_address){
            // Hvis MIP-adressen allerede finnes i cachen, oppdater MAC-adresse og grensesnitt-informasjon
            if(mac_address!=NULL){
                memcpy(current->mac_address, mac_address, sizeof(current->mac_address));
            }
            if(if_addr!=NULL){
                current->if_addr = if_addr;  // Oppdater grensesnittinformasjon
            }
 
            return 1;  // Returner 1 for å indikere at oppføringen ble oppdatert
        }
        current = current->next;
    }

    // Hvis oppføringen ikke finnes i cachen, opprett en ny
    struct entry *newEntry = malloc(sizeof(struct entry));
    if (newEntry == NULL){
        perror("Unable to allocate memory for new cache entry");
        return -1;  // Returner -1 for feil
    }

    // Sett opp nye verdier
    newEntry->mip_address = mip_address;
    if(mac_address!=NULL){
        memcpy(newEntry->mac_address, mac_address, sizeof(newEntry->mac_address));
    }
    if (if_addr!=NULL){
        newEntry->if_addr = if_addr;
    }

    newEntry->next = NULL;  // Sørg for at neste peker er NULL
    newEntry->queued_pdu_not_yet_sent = NULL;  // Ingen PDU i kø ved opprettelse

    // Legg til i cache-listen
    if (cache->head == NULL){
        // Hvis cachen er tom, sett denne oppføringen som både head og tail
        cache->head = newEntry;
        cache->tail = newEntry;
    } else {
        // Hvis cachen ikke er tom, legg den til på slutten av listen
        cache->tail->next = newEntry;
        cache->tail = newEntry;
    }

    return 1;  // Returner 1 for suksess
}

// Funksjon for å legge til en PDU i køen til en spesifikk MIP-adresse
int add_pdu_to_queue(struct cache *cache, uint8_t mip_address, struct mip_pdu* pdu) {
    struct entry *current = cache->head;

    // Gå gjennom cache-listen for å finne matchende MIP-adresse
    while (current != NULL) {
        if (current->mip_address == mip_address) {
            current->queued_pdu_not_yet_sent = pdu;  // Legg PDU til køen
            printf("PDU added to queue for mip_address: %d\n", mip_address);
            return 1;  // Returner 1 for suksess
        }
        current = current->next;
    }

    // Hvis MIP-adressen ikke finnes i cachen, returner -1
    printf("Could not find entry for mip_address: %d\n", mip_address);
    return -1;
}

// Funksjon for å hente en usendt PDU fra cachen
struct mip_pdu *fetch_queued_pdu_in_cache(struct cache *cache, uint8_t mip_address) {
    struct entry *current = cache->head;

    // Gå gjennom cache-listen for å finne matchende MIP-adresse
    while(current != NULL){
        if (current->mip_address == mip_address && current->queued_pdu_not_yet_sent != NULL){
            struct mip_pdu *pdu = current->queued_pdu_not_yet_sent;  // Hent PDU-en
            current->queued_pdu_not_yet_sent = NULL;  // Fjern PDU fra køen
            return pdu;  // Returner den usendte PDU-en
        }
        current = current->next;
    }

    return NULL;  // Returner NULL hvis ingen usendt PDU finnes
}
