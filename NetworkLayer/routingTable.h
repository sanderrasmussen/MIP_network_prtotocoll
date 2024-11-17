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

#define R 0x52
#define E 0x45
#define Q 0x51
#define S 0x53
#define P 0x50
#define HELLO 0x60
#define UPDATE 0x61 //these are for mipd to distinguish 

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
struct update_message {
    uint8_t packet_type;
    uint8_t src_mip_addr;
    uint8_t route_count;
    struct update_entry *routes; //dest, and cost
   

};
struct update_entry{
    uint8_t dest_mip_addr;
    uint8_t cost;
};
struct routingTable * create_routing_table() ;
int add_or_update_route(struct routingTable *table, uint8_t dest, uint8_t next_hop, uint8_t cost) ;
void print_routing_table(struct routingTable *table) ;
void print_routing_table(struct routingTable *table) ;
uint8_t get_next_hop(struct routingTable *table , uint8_t ttl, uint8_t dst_mip);

struct RoutingRequest *deserialize_request(char * payload);
struct RoutingResponse *deserialize_response(char * payload);
char *serialize_response(struct RoutingResponse response);
// Initialize a RoutingResponse

// Initialize a RoutingRequest
struct RoutingRequest create_routing_request(uint8_t src_addr, uint8_t dest_addr, uint8_t ttl) ;
// Initialize a RoutingResponse
struct RoutingResponse create_routing_response(uint8_t src_addr, uint8_t next_hop, uint8_t ttl) ;
char * serialize_router_requests(struct RoutingRequest request);
char * serialize_update_message(struct update_message* update_message) ;
struct update_message *deserialize_update_message(char * serialized_msg);
//fetches all routes and costs in table and creates a update_message struct and returns it
struct update_message * create_update_message(struct routingTable *table, uint8_t src_mip_addr) ;// set to 255 when in router, mipd will then change it to its own when relaying it
int update_table(struct routingTable *table,struct update_message* update);

#endif //end guard