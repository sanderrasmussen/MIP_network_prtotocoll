
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/stat.h>
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
#include "Application_layer/unix_socket.h"
#include "Link_layer/raw_socket.h"
#include "NetworkLayer/cache.h"

#define MAX_EVENTS 10   



// sending pong response from server to mipd, then the mipd should relay it to its destination over the network
void send_pong_response(int unix_socket, struct mip_client_payload *received_payload) {

    char pong_message[100];  
    
    snprintf(pong_message, sizeof(pong_message)-2, "PONG:%s", received_payload->message);


    struct mip_client_payload pong_payload;
    pong_payload.dst_mip_addr = received_payload->dst_mip_addr;
    pong_payload.message = pong_message;

    // Send PONG back
    int status = unixSocket_send(unix_socket, &pong_payload, 100);
    if(status==-1){
        perror("could not send \n");
    }

}

// handling mipd connections, these connections will all send PINGs
void handle_client_message(int client_fd) {
    // Motta klientens melding
    struct mip_client_payload received_payload;
    char buffer[100];
    memset(buffer, 0, sizeof(buffer));

    int bytes_read = read(client_fd, buffer, 100);


    received_payload.dst_mip_addr = buffer[0]; 
    received_payload.message = buffer + 1;    

    printf("Received message from MIP daemon: %s\n", received_payload.message);

    send_pong_response(client_fd, &received_payload);
    printf("pong response sent \n");
    
}

//Handle queued epoll socket events, mipd ping messages
void handle_events(int epoll_fd, int unix_socket) {
    struct epoll_event events[MAX_EVENTS];
    int num_ready, i;

    while (1) {
        num_ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_ready == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < num_ready; i++) {
            if (events[i].data.fd == unix_socket) {

                int client_fd = accept(unix_socket, NULL, NULL);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }
                printf("New client connected\n");

                handle_client_message(client_fd);
            }
        }
    }
}
// main function listening on unix socket for mipd PINGs
int main(int argc, char *argv[]) {

    if (argc != 2 || strcmp(argv[1], "-h") == 0) {
        printf("Usage: mipd [-h] [-d] <socket_upper> <MIP address>");
        exit(EXIT_FAILURE);
    }
    printf("server started \n");
    
    char *server_sock_path= malloc(sizeof("UsockX_server\0")); //standard 
    char *mipd_sock_path = argv[1];
    strcpy(server_sock_path, mipd_sock_path);
    strcat(server_sock_path, "_server");

    struct sockaddr_un address;

    int mask = umask(0);
    unlink(server_sock_path);
    int unix_socket = setupUnixSocket(server_sock_path, &address);
    unixSocket_bind(unix_socket, server_sock_path, &address);
    unixSocket_listen(unix_socket, server_sock_path, unix_socket);
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
        perror("epoll_ctl");
        close(unix_socket);
        exit(EXIT_FAILURE);
    }

    handle_events(epoll_fd, unix_socket);
    close(unix_socket);

    return 0;
}
