
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



// Funksjon for å lage en "PONG:<received message>"
void send_pong_response(int unix_socket, struct mip_client_payload *received_payload) {
    // Lag "PONG:<message>"
    char pong_message[256];  // Buffer for PONG-meldingen
    snprintf(pong_message, sizeof(pong_message), "PONG:%s", received_payload->message);

    // Lag ny payload med PONG-meldingen
    struct mip_client_payload pong_payload;
    pong_payload.dst_mip_addr = received_payload->dst_mip_addr;
    pong_payload.message = pong_message;

    // Send PONG-meldingen tilbake
    unixSocket_send(unix_socket, &pong_payload, strlen(pong_payload.message));
}

// Håndterer klientmeldinger
void handle_client_message(int client_fd) {
    // Motta klientens melding
    struct mip_client_payload received_payload;
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));

    int bytes_read = read(client_fd, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        // Ekstraher melding og destinasjons-MIP-adresse
        received_payload.dst_mip_addr = buffer[0]; // Første byte er dst_mip_addr
        received_payload.message = buffer + 1;     // Meldingen følger etter dst_mip_addr

        printf("Received message from MIP daemon: %s\n", received_payload.message);

        // Sende PONG-svar
        send_pong_response(client_fd, &received_payload);
    } else if (bytes_read == 0) {
        printf("Client disconnected\n");
    } else {
        perror("Error reading from socket");
    }
    close(client_fd);
}

// Håndterer hendelser via epoll
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
                // Accept en ny tilkobling
                int client_fd = accept(unix_socket, NULL, NULL);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }
                printf("New client connected\n");

                // Håndter klientmeldingen
                handle_client_message(client_fd);
            }
        }
    }
}

int main(int argc, char *argv[]) {
 
    if (argc != 2 || strcmp(argv[1], "-h") == 0) {
        printf("Usage: %s <socket_lower>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    printf("server started \n");
    // Argument: stien til Unix-socket
    char *socket_path = argv[1];
    struct sockaddr_un address;

    // Setup Unix-socket
    int unix_socket = setupUnixSocket(socket_path, &address);
    unixSocket_bind(unix_socket, socket_path, &address);
    unixSocket_listen(unix_socket, socket_path, unix_socket);

    // Set up epoll
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

    // Håndter innkommende hendelser
    handle_events(epoll_fd, unix_socket);

    close(unix_socket);
    unlink(socket_path);
    return 0;
}
