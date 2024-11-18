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

// For sending ping message to mipd
void send_ping_message( uint8_t dst_mip_addr, const char *message, char *socket_path, uint8_t ttl) {
        // Setup Unix-socket for sending messages to the mipd
    struct sockaddr_un address;
    int unix_socket = setupUnixSocket(socket_path, &address);
    int status = unixSocket_connect(unix_socket, socket_path, &address);

    struct mip_client_payload * payload=malloc(100);
    payload->dst_mip_addr = dst_mip_addr;
    payload->message = message;
    payload->ttl = ttl;
    status = unixSocket_send(unix_socket, payload, strlen(payload->message)+(sizeof(uint8_t)*2));
    if(status==-1){
        perror("send");
        exit(EXIT_FAILURE);
    }
    printf("Ping message : %s sent \n ", message);
    printf("sent to : %d \n", dst_mip_addr);
    free(payload);
    close(unix_socket);
}
void receive_pong(char * socket_path){

    //closing the sending socket and creating a listening socket
    char *pong[200];

    //create new recv socket for listening to pongs :
    char* recv_sock_path = malloc(strlen("usockX_client\0")); //usockAclient
    memcpy(recv_sock_path, socket_path, strlen(socket_path));
    strcat(recv_sock_path, "_client\0");
    unlink(recv_sock_path);
    struct sockaddr_un recv_addr;
    int recv_sock = setupUnixSocket(recv_sock_path, &recv_addr);
    unixSocket_bind(recv_sock, recv_sock_path, &recv_addr);
    unixSocket_listen(recv_sock, recv_sock_path, 5);  // Backlog of 5

    //watitng for mipd to connect and send pong
    int accept_sock = accept(recv_sock, NULL, NULL);
    if (accept_sock == -1) {
        perror("accept failed");
        close(recv_sock);
        unlink(recv_sock_path);
        exit(EXIT_FAILURE);
    }

    // Receive PONG
    memset(pong, 0, sizeof(pong)); 
    int bytes_read = read(accept_sock, pong, sizeof(pong) - 1);  

    printf("Pong received: %s\n", pong  );
    write(accept_sock,"ACK\0",5);
    //clean up
    close(accept_sock);
    close(recv_sock);
    unlink(recv_sock_path);
}

/* Sadly i am unable to use the same socket as received in argument when receiving pong. The mipd is already listening on it and i have implemented
it in a way where it does not serve only one client at a time, but using FIFO scheduling. Therefore i must make a new socket path: 3.g. usockAclient for receving pongs from mipd*/
int main(int argc, char *argv[]) {
    if (argc != 5 || strcmp(argv[1], "-h") == 0) {
        printf("Usage: %s <socket_path> <dst_mip_addr> <message>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    printf("Started MIP application \n");
    char *socket_path = argv[1];
    uint8_t ttl = argv[4]; 
    uint8_t dst_mip_addr = (uint8_t)atoi(argv[2]);
    char *message = malloc(99);
    strcpy(message, argv[3]); // this should null terminate the string 


    // Send ping to MIP daemon
    send_ping_message( dst_mip_addr, message, socket_path,ttl );
    receive_pong(socket_path);
  
    return 0;
}
