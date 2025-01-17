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
#include "routingd.h"


#define HELLO_INTERVAL 10      
#define UPDATE_INTERVAL 15     
#define MAX_EVENTS 10          

// sendts payload to mipd
int send_to_mipd(char * payload, char *mipd_path){
    struct sockaddr_un *addr = malloc(sizeof(struct sockaddr_un));
    int sock = setupUnixSocket(mipd_path, addr);
    unixSocket_connect(sock,mipd_path, addr);
    unixSocket_send_String(sock,payload,strlen(payload));
    close(sock);
    free(addr);
}


// function for sending hello message 
void send_hello_message(char *socket_path) {
    struct sockaddr_un address;
    printf("Sending HELLO message...\n");
    printf("socket path %s", socket_path);
    // Setup Unix-socket for sending messages to the mipd
    int mipd_unix_socket = setupUnixSocket(socket_path, &address);
    int status = unixSocket_connect(mipd_unix_socket, socket_path, &address);
    if (status == -1){
        perror("connect client");
    }
    //send HEllo
    char *payload = "HELLO\0";
    status = unixSocket_send_String(mipd_unix_socket, payload, strlen(payload));
    if(status<0){
        perror("unix socket send");
    }
    printf(" hello message sent \n");
    
}


// setting up timer socket
int setup_periodic_timer(int interval) {
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd == -1) {
        perror("timerfd_create");
        exit(EXIT_FAILURE);
    }

    struct itimerspec timer_spec;
    timer_spec.it_interval.tv_sec = interval;
    timer_spec.it_interval.tv_nsec = 0;
    timer_spec.it_value.tv_sec = interval;
    timer_spec.it_value.tv_nsec = 0;

    if (timerfd_settime(timer_fd, 0, &timer_spec, NULL) == -1) {
        perror("timerfd_settime");
        close(timer_fd);
        exit(EXIT_FAILURE);
    }

    return timer_fd;
}

// handling incomming requests on sockets
int handle_request(int unix_socket, struct routingTable *routingTable, char *mipd_path) {
    printf("Handling incoming request...\n");
    // Implementer logikken for behandling av forespørsler her
    char *payload = malloc(100);

    read(unix_socket, payload,200);
    if (strncmp(payload, "HELLO", 5) == 0){
        uint8_t mip_addr = *(uint8_t *)(payload + 5);
        printf("====== discovered new host %d ====== \n", mip_addr);
        add_or_update_route(routingTable,mip_addr,mip_addr,1);
        printf("    added %d to table \n", mip_addr);
        print_routing_table(routingTable);
    }
    else if (payload[0]==UPDATE){

        //deserialize into update_message
        struct update_message * update_msg = deserialize_update_message(payload);
        update_table(routingTable,update_msg);
        return 1;
    }
    //if request 
    else if (payload[2]==R && payload[3] == E && payload[4]== Q){
        // we need to make sure cost of route is equal or smaller than ttl
        struct RoutingRequest *request = deserialize_request(payload);
        //find route
        printf("SEARCHING FOR ADDR %d \n", request->dest_mip_addr);
        uint8_t next_hop = get_next_hop(routingTable,request->ttl,request->dest_mip_addr);
        if (next_hop==255){
            printf("coukd not find next hop");
            return -1;
        }
        printf("NEXT HOP FOUND : %d\n", next_hop);
        // make response containign next hop.
        struct RoutingResponse response = create_routing_response(request->src_mip_addr,next_hop,1);
        char * serialized_response = serialize_response(response);
        printf("serialized dst %d \n", serialized_response[5]);
        //send response back to mipd
        write(unix_socket,serialized_response, sizeof(struct RoutingResponse));
        //send_to_mipd(serialize_response, mipd_path);
        printf("RESPONSE SENT TO MIPD \n");
    }   
    return 1;
    
}


// handles epoll events
void handle_router_events(int epoll_fd, int unix_socket, int hello_timer_fd, int update_timer_fd, char * socket_path, char *mipd_socket_path, struct sockaddr_un *address, struct routingTable *routingTable, int remove_stale_timer) {
    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == unix_socket) {
                int client_fd = accept(unix_socket, NULL, NULL);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }
                printf("New client connected\n");
                handle_request(client_fd, routingTable,mipd_socket_path);
                close(client_fd);
            } else if (events[i].data.fd == hello_timer_fd) {
                // Timer for HELLO utløpt - send HELLO-melding
                uint64_t expirations;
                read(hello_timer_fd, &expirations, sizeof(expirations)); // Tømmer timeren
                send_hello_message(mipd_socket_path);
                    print_routing_table(routingTable);
                
            } else if (events[i].data.fd == update_timer_fd) {
                // Timer for UPDATE utløpt - send UPDATE-melding
                uint64_t expirations;
                read(update_timer_fd, &expirations, sizeof(expirations)); // Tømmer timeren
                advertise_routes(routingTable, mipd_socket_path);


            }
            else if(events[i].data.fd == remove_stale_timer){
                  // Timer for å fjerne utdaterte ruter utløst
                uint64_t expirations;
                read(remove_stale_timer, &expirations, sizeof(expirations)); // Tømmer timeren
                printf("Checking for stale routes...\n");
                remove_stale_routes(routingTable);
                print_routing_table(routingTable); 
            }
        }
    }
}
//sends routes to mipd so that it can advertise the routes
int advertise_routes(struct routingTable * routingTable, char *socket_path){
    //map routes and create message
    struct update_message *update_msg = create_update_message(routingTable,255);//temp 255 becvause routerd does not know mip addr of mipd, mipd wil set the address in the packet
    //serialize and send to mipd
    char * serialized_update_msg=serialize_update_message(update_msg);
    //send to mips
    struct sockaddr_un address;
    printf("Sending UPDATE message...\n");
    int mipd_unix_socket = setupUnixSocket(socket_path, &address);
    int status = unixSocket_connect(mipd_unix_socket, socket_path, &address);
    if (status == -1){
        perror("connect client");
    }
    unixSocket_send_String(mipd_unix_socket, serialized_update_msg, strlen(serialized_update_msg));
    close(mipd_unix_socket);
    printf("UPPDATE message sent \n");

}



// main function of routingd
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: routingd [-d] <socket_upper>\n");
        exit(EXIT_FAILURE);
    }

    printf("Routing daemon started\n");
    char *mipd_socket_path = argv[2];
    printf("MIPD socket path: %s\n", mipd_socket_path);

    char socket_path[strlen(argv[2]) + strlen("_routingd") + 1];
    snprintf(socket_path, sizeof(socket_path), "%s_routingd", argv[2]);

    struct sockaddr_un address;
    int mask = umask(0);
    unlink(socket_path);
    int unix_socket = setupUnixSocket(socket_path, &address);
    unixSocket_bind(unix_socket, socket_path, &address);
    unixSocket_listen(unix_socket, socket_path, unix_socket);
    umask(mask);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    event.events = EPOLLIN;

    event.data.fd = unix_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, unix_socket, &event) == -1) {
        perror("epoll_ctl (unix_socket)");
        close(unix_socket);
        exit(EXIT_FAILURE);
    }

    //timers
    int hello_timer_fd = setup_periodic_timer(HELLO_INTERVAL);
    event.data.fd = hello_timer_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, hello_timer_fd, &event) == -1) {
        perror("epoll_ctl (hello_timer_fd)");
        close(unix_socket);
        close(hello_timer_fd);
        exit(EXIT_FAILURE);
    }

    int update_timer_fd = setup_periodic_timer(UPDATE_INTERVAL);
    event.data.fd = update_timer_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, update_timer_fd, &event) == -1) {
        perror("epoll_ctl (update_timer_fd)");
        close(unix_socket);
        close(hello_timer_fd);
        close(update_timer_fd);
        exit(EXIT_FAILURE);
    }

    int remove_stale_timer_fd = setup_periodic_timer(REMOVE_STALE_INTERVAL);
    event.data.fd = remove_stale_timer_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, remove_stale_timer_fd, &event) == -1) {
        perror("epoll_ctl (remove_stale_timer_fd)");
        close(unix_socket);
        close(remove_stale_timer_fd);
        exit(EXIT_FAILURE);
    }

    struct routingTable *routingTable = create_routing_table();

    send_hello_message(mipd_socket_path);

    handle_router_events(epoll_fd, unix_socket, hello_timer_fd, update_timer_fd, 
                         socket_path, mipd_socket_path, &address, routingTable, remove_stale_timer_fd);

    close(unix_socket);
    close(hello_timer_fd);
    close(update_timer_fd);
    close(remove_stale_timer_fd);
    return 0;
}
