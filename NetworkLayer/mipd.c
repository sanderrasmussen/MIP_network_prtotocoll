#define ETH_P_MIP 0x88B5
#define MIP_ADDRESS 255

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/stat.h>
#include "mipd.h"
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
#include "../Application_layer/unix_socket.h"
#include "../Link_layer/raw_socket.h"

/*  if(VARIBLE==-1){
        fprintf(stderr, "Error: XXXX");
        exit(-1);
    }
*/
int serve_raw_connection( int raw_socket){
    char buffer[1024];
    recv_raw_packet(raw_socket,buffer, 1024);

}
int serve_unix_connection(struct epoll_event *events,int sock_accept, int raw_socket, struct sockaddr_ll *socket_name){
    /* an already existing client is trying to send packets*/
    struct mip_client_packet *buffer=malloc(sizeof(struct mip_client_packet));
    int rc;
    //set all in buffer to 0
    //memset(buffer, 0, sizeof(buffer)); THIS CAUSES SEGFAULT
    
    /* when receiving from client, it is only to be forwarded, we check cahce, if no mac we send arp*/
    rc = read(events->data.fd, buffer, sizeof(struct mip_client_packet));
    if (rc == -1) {
        close(events->data.fd);
        printf("<%d> left the chat...\n", events->data.fd);
        return -1;
    }
    //unpack unix socket message
    printf("\n");
    printf("===mip address: %d", buffer->dst_mip_addr );
    printf("message: %s ===", buffer->message);
    send_arp(raw_socket,socket_name);
    printf("\n");
    close(events->data.fd);
    
    return 1;
};


void handle_events(int socket,int raw_socket, struct sockaddr_ll *socket_name){
    int status, readyIOs;
    struct epoll_event event, events[10];//10 is max events
    int epoll_socket;
    int sock_accept;

    /* epoll file descriptor for event handlings */
	epoll_socket = epoll_create1(0);
	if (epoll_socket == -1) {
		perror("epoll_create1");
		close(socket );
		exit(EXIT_FAILURE);
	}

    //adding the connection coket to table 
    status= add_to_epoll_table(epoll_socket, &event,socket);
    if(status==-1){
        perror("add to epoll table failed");
        exit(EXIT_FAILURE);
    }
    //adding the raw socket to table
    status = add_to_epoll_table(epoll_socket, &event, raw_socket);
    if(status==-1){
        perror("adding raw socket to epol table");
        exit(EXIT_FAILURE);
    }
    for(;;){
        readyIOs= epoll_wait(epoll_socket, events, 10, -1);
        if (status==-1){
            perror("epoll wait error");
            close(socket);
            exit(EXIT_FAILURE);
        }
        for(int i = 0; i<readyIOs; i++){

            /* If someone is trying to connect to unix socket*/
            if (events[i].data.fd == socket){
                sock_accept= accept(events->data.fd,NULL,NULL);
                if (sock_accept== -1){
                    perror("error on accept connection");
                    continue;
                }

                printf("client connected");

                status = add_to_epoll_table(epoll_socket, &event , sock_accept);
                if (status== -1){
                    close(socket);
                    perror("add to epoll table error");
                    exit(EXIT_FAILURE);
                }
            }
            //handle raw socket connections, like arp broadcasts and so on
            else if(events[i].data.fd== raw_socket){
                serve_raw_connection(events[i].data.fd);
            }
            //someone is connected to unix socket and wants to send data
            else{
                serve_unix_connection(events, sock_accept, raw_socket, socket_name);
            }
        }
    }
}
int main(int argc, char *argv[]){ 

    int raw_socket;
    struct sockaddr_ll *socket_name=malloc(sizeof(struct sockaddr_ll));
    raw_socket = setupRawSocket();
    get_mac_from_interface(socket_name);
    //send_arp(raw_socket, socket_name); //this one will be in handle events later, currenntly here for testing

    int unix_connection_socket ;
    int unix_data_socket;
    int status;
    struct sockaddr_un *address = malloc(sizeof(struct sockaddr_un)) ;
    if (address==NULL){
        perror("could not malloc address");
        exit(0);
    }
    char *pathToSocket = "/tmp/unix.sock";

    unlink(pathToSocket);
    char *buffer;
    unix_connection_socket = setupUnixSocket(pathToSocket, address);
    unix_data_socket = unixSocket_bind(unix_connection_socket, pathToSocket, address );
    status = unixSocket_listen( unix_connection_socket, buffer, unix_data_socket);

    handle_events(unix_connection_socket,raw_socket, socket_name);

    close(raw_socket);
    close(unix_connection_socket);
    close(unix_data_socket);
    unlink(pathToSocket);
    free(address);
    exit(1);
    return 1;
};

//raw mip ethernet communication

struct mip_pdu* create_mip_datagram(struct mip_client_packet *client_packet, uint8_t sdu_type){
    //fill mip header
    struct mip_header * header = malloc(sizeof(struct mip_header));
    //filling the header values
    memcpy(header->dest_addr, client_packet->dst_mip_addr, 8);
    header->src_addr = MIP_ADDRESS;
    header->ttl= 1; //recommended to set to one according to exam text
    header->sdu_len = strlen(client_packet->message);
    header->sdu_type= sdu_type;

    //adding the payload 
    struct mip_pdu* mip_pdu = malloc(sizeof(struct mip_header)+ header->sdu_len*sizeof(char));
    memcpy(&mip_pdu->mip_header, header, sizeof(struct mip_header));
    memcpy(mip_pdu->sdu, client_packet->message, header->sdu_len);

    return mip_pdu;

}
//to be implemented.
int get_mac_from_cache(uint8_t mip_address){
    return -1;
}
int add_to_cache(uint8_t mip_address, uint8_t mac_address ){

}