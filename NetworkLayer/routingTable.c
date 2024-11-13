#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "routingTable.h"


// Funksjon for å opprette en tom rutetabell
struct routingTable * create_routing_table() {
    struct routingTable *table = malloc(sizeof(struct routingTable));
    table->route_count = 0;
    for (int i = 0; i < MAX_ROUTES; i++) {
        table->routes[i].dest = 255;    // Setter 255 som en verdi for "ikke-brukte" ruter
        table->routes[i].cost = INFINITY;
    }
    return table;
}


// Funksjon for å legge til eller oppdatere en rute i rutetabellen
int add_or_update_route(struct routingTable *table, uint8_t dest, uint8_t next_hop, uint8_t cost) {
    // Sjekker om ruten eksisterer fra før
    for (int i = 0; i < table->route_count; i++) {
        if (table->routes[i].dest == dest) {
            // Oppdaterer ruten hvis den eksisterer, og den nye kostnaden er lavere
            if (cost < table->routes[i].cost) {
                table->routes[i].next_hop = next_hop;
                table->routes[i].cost = cost;
                return 1;  // Returnerer 1 for oppdatert rute
            }
            return 0;      // Ruten eksisterer allerede med samme eller lavere kostnad
        }
    }

    // Legger til ny rute hvis destinasjonen ikke finnes i tabellen
    if (table->route_count < MAX_ROUTES) {
        table->routes[table->route_count].dest = dest;
        table->routes[table->route_count].next_hop = next_hop;
        table->routes[table->route_count].cost = cost;
        table->route_count++;
        return 1;  // Returnerer 1 for ny rute
    }
    
    return -1; // Tabell er full
}

// Funksjon for å fjerne en rute fra rutetabellen
int remove_route(struct routingTable *table, uint8_t dest) {
    for (int i = 0; i < table->route_count; i++) {
        if (table->routes[i].dest == dest) {
            // Fjerner ruten ved å flytte siste element inn her
            table->routes[i] = table->routes[table->route_count - 1];
            table->route_count--;
            return 1;  // Returnerer 1 for suksessfull fjerning
        }
    }
    return 0;  // Returnerer 0 hvis ruten ikke finnes
}
//searches table for suitable routes and returns mip address of next hop if found suitable route to dst. On failure returns -1
uint8_t get_next_hop(struct routingTable *table , uint8_t ttl, uint8_t dst_mip){
    //simply loop and find route entry to destination
    for (int i = 0; i < table->route_count; i++){
        if (table->routes[i].dest == dst_mip && table->routes[i].cost <= ttl){
            printf("'''''' fetchewd next hop: %d\n", table->routes[i].next_hop);
            return table->routes[i].next_hop;//returning mip address
        }
    }
    return 255; //no suitable route found
    
}
// Funksjon for å skrive ut rutetabellen for debugging
void print_routing_table(struct routingTable *table) {
    printf("Routing Table:\n");
    printf("Dest\tNext Hop\tCost\n");
    for (int i = 0; i < table->route_count; i++) {
        printf("%d\t%d\t\t%d\n", table->routes[i].dest, table->routes[i].next_hop, table->routes[i].cost);
    }
}


struct RoutingRequest *deserialize_request(char * payload){
    struct RoutingRequest * res = malloc(sizeof(struct RoutingRequest));
    res->src_mip_addr = payload[0];
    res->ttl = payload[1];
    res->r = payload[2];
    res->e = payload[3];
    res->q = payload[4];
    res->dest_mip_addr = payload[5];

    return res;

}
struct RoutingResponse *deserialize_response(char * payload){
    struct RoutingResponse* res = malloc(sizeof(struct RoutingResponse));
    res->src_mip_addr = payload[0];
    res->ttl = payload[1];
    res->r = payload[2];
    res->s = payload[3];
    res->p = payload[4];
    res->next_hop_mip_addr = payload[5];

    return res;

}
char *serialize_response(struct RoutingResponse response){
    char *buffer= malloc(sizeof(struct RoutingResponse));
    buffer[0]= response.src_mip_addr;
    buffer[1]= response.ttl;
    buffer[2] =response.r;
    buffer[3] = response.s;
    buffer[4] = response.p;
    buffer[5] = response.next_hop_mip_addr;
    return buffer;
}

char * serialize_router_requests(struct RoutingRequest request){
    char *buffer= malloc(sizeof(struct RoutingRequest));
    buffer[0]= request.src_mip_addr;
    buffer[1]= request.ttl;
    buffer[2] =request.r;
    buffer[3] = request.e;
    buffer[4] = request.q;
    buffer[5] = request.dest_mip_addr;
    return buffer;
}


// Initialize a RoutingRequest
struct RoutingRequest create_routing_request(uint8_t src_addr, uint8_t dest_addr, uint8_t ttl) {
    struct RoutingRequest request;
    request.src_mip_addr = src_addr;
    request.ttl = ttl;
    request.r = R;  // 'R'
    request.e = E;  // 'E'
    request.q = Q;  // 'Q'
    request.dest_mip_addr = dest_addr;
    return request;
}
// Initialize a RoutingResponse
struct RoutingResponse create_routing_response(uint8_t src_addr, uint8_t next_hop, uint8_t ttl) {
    struct RoutingResponse res;
    res.src_mip_addr = src_addr;
    res.ttl = ttl;
    res.r = R;
    res.s = S;
    res.p = P;
    res.next_hop_mip_addr= next_hop;
    return res;
}
