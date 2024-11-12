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
uint8_t get_best_route(struct routingTable *table , uint8_t ttl, uint8_t dst_mip){
    //simply loop and find route entry to destination
    for (int i = 0; i < table->route_count; i++){
        if (table->routes[i].dest == dst_mip && table->routes[i].cost <= ttl){
            return table->routes[i].dest;//returning mip address
        }
    }
    return -1; //no suitable route found
    
}
// Funksjon for å skrive ut rutetabellen for debugging
void print_routing_table(struct routingTable *table) {
    printf("Routing Table:\n");
    printf("Dest\tNext Hop\tCost\n");
    for (int i = 0; i < table->route_count; i++) {
        printf("%d\t%d\t\t%d\n", table->routes[i].dest, table->routes[i].next_hop, table->routes[i].cost);
    }
}

