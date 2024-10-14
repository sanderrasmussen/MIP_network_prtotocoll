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
    printf("Ping message : %s sent \n ", message);
    printf("sent to : %d \n", dst_mip_addr);
  // Frigjør minne etter bruk
}



int main(int argc, char *argv[]) {
    if (argc != 4 || strcmp(argv[1], "-h") == 0) {
        printf("Usage: %s <socket_path> <dst_mip_addr> <message>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    printf("Started MIP application \n");
    char *socket_path = argv[1];
    uint8_t dst_mip_addr = (uint8_t)atoi(argv[2]);
    char *message = argv[3];

    struct sockaddr_un address;

    // Setup Unix-socket for å sende og motta meldinger
    int unix_socket = setupUnixSocket(socket_path, &address);
    unixSocket_connect(unix_socket, socket_path, &address);

    // Send ping-melding til MIP daemon
    send_ping_message(unix_socket, dst_mip_addr, message);
    close(unix_socket);
    //closing the sending socket and creating a listening socket
    char *pong[200];
    //create new recv socket :
    int recv_sock = setupUnixSocket(socket_path, &address);
    unixSocket_bind(recv_sock, socket_path, &address);
    unixSocket_listen(recv_sock, socket_path, 5);  // Backlog settes til 5

    // Vent på tilkobling fra MIP daemon
    int accept_sock = accept(recv_sock, NULL, NULL);
    if (accept_sock == -1) {
        perror("accept failed");
        close(recv_sock);
        unlink(socket_path);
        exit(EXIT_FAILURE);
    }

    // Motta PONG-melding

    memset(pong, 0, sizeof(pong));  // Initialiser mottaksbufferet
    int bytes_read = read(accept_sock, pong, sizeof(pong) - 1);  // -1 for å sikre plass til null-terminator

    printf("Pong received: %s\n", pong  );
    write(accept_sock,"ACK",3);
    // Lukk socket etter å ha mottatt svaret
    close(accept_sock);
    close(recv_sock);
    unlink(socket_path);  // Fjern socket-filen
    return 0;
}
