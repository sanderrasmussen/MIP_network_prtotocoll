#ifndef ROUTINGD_H_   /* Include guard */
#define ROUTINGD_H_

#include <stdint.h>


#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/stat.h>
#include "mipd.h"
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <time.h>
#include <sys/timerfd.h>
#include "../Application_layer/unix_socket.h"
#include "../Link_layer/raw_socket.h"
#include "cache.h"
#include "routingTable.h"



#define HELLO_INTERVAL 10      // HELLO-melding hvert 10. sekund
#define UPDATE_INTERVAL 30     // UPDATE-melding hvert 30. sekund
#define MAX_EVENTS 10          // Maks antall events vi kan håndtere samtidig

int send_to_mipd(char * payload, char *mipd_path);
// Funksjon for å sende en HELLO-melding
void send_hello_message(char *socket_path) ;
// Funksjon for å sende en UPDATE-melding
void send_update_message(  char* socket_path) ;
// Funksjon for å sette opp en periodisk timer
int setup_periodic_timer(int interval) ;
// Funksjon for å håndtere innkommende forespørsler
void handle_request(int unix_socket, struct routingTable *routingTable, char *mipd_path) ;
// Funksjon for å håndtere epoll-hendelser
void handle_router_events(int epoll_fd, int unix_socket, int hello_timer_fd, int update_timer_fd, char * socket_path, char *mipd_socket_path, struct sockaddr_un *address, struct routingTable *routingTable) ;


#endif // MIP_H_