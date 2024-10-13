#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>  /* free */
#include <stdio.h>   /* printf */
#include <unistd.h>  /* fgets */
#include <string.h>  /* memset */
#include <sys/socket.h>  /* socket */
#include <fcntl.h>
#include <sys/epoll.h>    /* epoll */
#include <linux/if_packet.h>  /* AF_PACKET */
#include <net/ethernet.h> /* ETH_* */
#include <arpa/inet.h> /* htons */
#include <ifaddrs.h>   /* getifaddrs */
#include "Application_layer/unix_socket.h"

#define MAX_EVENTS 10

// Funksjon for å sende ping-melding til MIP daemon
void send_ping_message(int unix_socket, uint8_t dst_mip_addr, const char *message) {
    struct mip_client_payload payload;
    payload.dst_mip_addr = dst_mip_addr;
    payload.message = message;

    // Send meldingen til MIP daemon
    unixSocket_send(unix_socket, &payload, strlen(payload.message));
  // Frigjør minne etter bruk
}

// Funksjon for å motta svar fra MIP daemon
void handle_response(int client_fd) {
    // Motta svaret fra MIP daemon
    struct mip_client_payload received_payload;
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));

    int bytes_read = read(client_fd, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        received_payload.dst_mip_addr = buffer[0]; // Første byte er dst_mip_addr
        received_payload.message = buffer + 1;     // Meldingen følger etter dst_mip_addr

        printf("Received response from MIP daemon: %s\n", received_payload.message);
    } else if (bytes_read == 0) {
        printf("MIP daemon disconnected\n");
    } else {
        perror("Error reading from socket");
    }
}

// Håndterer hendelser via epoll
void wait_for_response(int epoll_fd, int unix_socket) {
    struct epoll_event events[MAX_EVENTS];
    int num_ready, i;

    // Vent på svar med epoll
    while (1) {
        num_ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_ready == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < num_ready; i++) {
            if (events[i].data.fd == unix_socket) {
                // Motta svaret fra MIP daemon
                handle_response(unix_socket);
                return;  // Avslutt etter å ha mottatt svar
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4 || strcmp(argv[1], "-h") == 0) {
        printf("Usage: %s <socket_path> <dst_mip_addr> <message>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *socket_path = argv[1];
    uint8_t dst_mip_addr = (uint8_t)atoi(argv[2]);
    char *message = argv[3];

    struct sockaddr_un address;

    // Setup Unix-socket for å sende og motta meldinger
    int unix_socket = setupUnixSocket(socket_path, &address);
    unixSocket_connect(unix_socket, socket_path, &address);

    // Send ping-melding til MIP daemon
    send_ping_message(unix_socket, dst_mip_addr, message);

    // Set up epoll for å vente på svar
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    event.events = EPOLLIN;  // Vi venter på innkommende data
    event.data.fd = unix_socket;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, unix_socket, &event) == -1) {
        perror("epoll_ctl");
        close(unix_socket);
        exit(EXIT_FAILURE);
    }

    // Vent på svar fra MIP daemon
    wait_for_response(epoll_fd, unix_socket);

    // Lukk socket etter å ha mottatt svar
    close(unix_socket);
    unlink(socket_path);
    return 0;
}
