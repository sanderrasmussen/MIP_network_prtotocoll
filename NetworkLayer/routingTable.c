#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "routingTable.h"
#include <sys/timerfd.h>
#include <poll.h>

#define ROUTE_TIMEOUT 30 // Maks tid uten oppdatering i sekunder
//adds route if not already in table. if in table it checks if the new route is better and if it is the new route replaces the old
int add_or_update_route(struct routingTable *table, uint8_t dest, uint8_t next_hop, uint8_t cost) {
    time_t current_time = time(NULL); // Hent nåværende tid
    for (int i = 0; i < table->route_count; i++) {
        if (table->routes[i].dest == dest) {
            // Oppdater ruten hvis kostnaden er lavere eller lik
            if (cost <= table->routes[i].cost) {
                table->routes[i].next_hop = next_hop;
                table->routes[i].cost = cost;
                table->routes[i].time_last_updated = current_time; // Oppdater tiden
                return 1;  // Returner 1 for oppdatert rute
            }
            return 0;      // Ruten eksisterer allerede med samme eller lavere kostnad
        }
    }

    // Legg til ny rute hvis destinasjonen ikke finnes i tabellen
    if (table->route_count < MAX_ROUTES) {
        table->routes[table->route_count].dest = dest;
        table->routes[table->route_count].next_hop = next_hop;
        table->routes[table->route_count].cost = cost;
        table->routes[table->route_count].time_last_updated = current_time; // Sett tid
        table->route_count++;
        return 1;  // Returner 1 for ny rute
    }
    
    return -1; // Tabell er full
}

//for removing routes that has not been checked through incomming update and hello messages in a while
void remove_stale_routes(struct routingTable *table) {
    time_t current_time = time(NULL); // Nåværende tid

    for (int i = 0; i < table->route_count; ) {
        if (difftime(current_time, table->routes[i].time_last_updated) > ROUTE_TIMEOUT) {
            // Fjern ruten hvis den er foreldet
            printf("Removing stale route to %d\n", table->routes[i].dest);
            table->routes[i] = table->routes[table->route_count - 1];
            table->route_count--;
        } else {
            i++; // Gå til neste rute
        }
    }
}

// timer for removing stale routes through disconnected clients
void periodic_cleaner_with_timerfd(struct routingTable *table) {
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd == -1) {
        perror("timerfd_create");
        return;
    }

    struct itimerspec timer_spec;
    timer_spec.it_interval.tv_sec = 10; // Periodisk hver 10. sekund
    timer_spec.it_interval.tv_nsec = 0;
    timer_spec.it_value.tv_sec = 10;    // Start med 10 sekunder
    timer_spec.it_value.tv_nsec = 0;

    if (timerfd_settime(timer_fd, 0, &timer_spec, NULL) == -1) {
        perror("timerfd_settime");
        close(timer_fd);
        return;
    }

    struct pollfd fds;
    fds.fd = timer_fd;
    fds.events = POLLIN;

    while (1) {
        int poll_result = poll(&fds, 1, -1);
        if (poll_result > 0 && (fds.revents & POLLIN)) {
            uint64_t expirations;
            read(timer_fd, &expirations, sizeof(expirations)); // Nullstiller timer
            remove_stale_routes(table);
        }
    }

    close(timer_fd);
}

// creates an empty routing table 
struct routingTable *create_routing_table() {
    //initialize all routes with hops, cost=infinity, nexthop =255
    struct routingTable *table = malloc(sizeof(struct routingTable));
    table->route_count = 0; // Ingen oppføringer ved start
    return table;
}
// updates the table if the given update message contains any routes that are better than the routes already in the table
int update_table(struct routingTable *table,struct update_message* update){
    //loop through all routes and compare to won, if better we uypdate our own
    struct update_entry * routes = update->routes;
    for(int i = 0; i<update->route_count; i++){
        add_or_update_route(table, routes[i].dest_mip_addr, update->src_mip_addr, routes[i].cost+1); //only cost+1 here since this is only used in combination with update messages
    }
    return 1;
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
            printf("    fetchewd next hop: %d\n", table->routes[i].next_hop);
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

// takes the buffer from mipd and deserializes it into a routing request
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
// takes the buffer from mipd and deserializes it into a routing response
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
// serializes a routing resposne to a buffer that can be sent to the mipd
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
// serializes a routing request to a buffer that can be sent to the mipd
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
// takes a given update message and serializes it into a buffer to be sent to the mipd
char * serialize_update_message(struct update_message* update_message) {
    // Beregn bufferstørrelsen: first byte indicates it is an update package using "#define UPDATE 1"  1 byte for routecount + 2 bytes per rute (dest_mip_addr + cost)
    size_t buffer_size =  1 + 1+ 1+(update_message->route_count * 2) ;// src addr + packetType + route count + route array 
    char *buffer = malloc(buffer_size);

    int pos =0; 
    // Sett update field i første byte av bufferet, derretter set route count
    buffer[pos++] = update_message->packet_type;
    buffer[pos++] = update_message->src_mip_addr;
    buffer[pos++] = update_message->route_count;
    // Serialiser rutene
    for (int i = 0; i < update_message->route_count; i++) {
        buffer[pos++] = update_message->routes[i].dest_mip_addr; // Legg til destinasjons-MIP
        buffer[pos++] = update_message->routes[i].cost;          // Legg til kostnad
    }

    return buffer; // Returnerer den serialiserte meldingen
}
// deserializes a buffer from mipd into a update message struct which is then returned
struct update_message *deserialize_update_message(char * serialized_msg){
    struct update_message *update_msg = malloc(sizeof(struct update_message));
    int pos= 0; //skipping the UPDATE packet type field
    update_msg->packet_type = serialized_msg[pos++];
    update_msg->src_mip_addr = serialized_msg[pos++];
    update_msg->route_count = serialized_msg[pos++];
    update_msg->routes= malloc(sizeof(struct update_entry)* update_msg->route_count);

    //deserialize routes
    for (int i = 0; i < update_msg->route_count; i++) {
        update_msg->routes[i].dest_mip_addr = serialized_msg[pos++]; // Legg til destinasjons-MIP
        update_msg->routes[i].cost= serialized_msg[pos++];          // Legg til kostnad
    }
    return update_msg;
}

//fetches all routes and costs in table and creates a update_message struct and returns it
struct update_message * create_update_message(struct routingTable *table, uint8_t src_mip_addr) {
    int update_size =  1 + 1+ 1+(table->route_count * 2);
    struct update_message *update_msg = malloc(update_size);

    update_msg->routes=  malloc(sizeof(struct update_entry)*(table->route_count));//we need mipaddr + routeCost
    update_msg->src_mip_addr = src_mip_addr;
    update_msg->packet_type=UPDATE;
    update_msg->route_count= table->route_count;

    int next_pos = 0;
    for (int i = 0; i < table->route_count; i++) {
        update_msg->routes[i].dest_mip_addr = table->routes[i].dest;  
        update_msg->routes[i].cost= table->routes[i].cost; 
    }

    return update_msg; 
}
