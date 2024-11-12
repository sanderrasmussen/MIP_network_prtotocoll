#ifndef ROUTINGTABLE_H_   /* Include guard */
#define ROUTINGTABLE_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_ROUTES 100       // Maks antall ruter som kan lagres
#define INFINITY 16          // Definerer 'infinite' metrikk for DVR-protokollen 15 er den storste tillatte kostnaden for en rute

struct routeEntry{
    uint8_t next_hop;
    uint8_t dest;
    uint8_t cost;
};
struct routingTable{
    struct routeEntry routes[MAX_ROUTES];
    int route_count;
};

struct routingTable * create_routing_table() ;
int add_or_update_route(struct routingTable *table, uint8_t dest, uint8_t next_hop, uint8_t cost) ;
void print_routing_table(struct routingTable *table) ;
void print_routing_table(struct routingTable *table) ;
uint8_t get_best_route(struct routingTable *table , uint8_t ttl, uint8_t dst_mip);


#endif //end guard