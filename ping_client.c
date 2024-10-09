
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

//returns message length with padding in bytes
size_t fill_sdu(struct mip_client_payload *packet,  char *message){
    //use trlen to find the length in bytes and add padding if necesary
    /* I assume that the sdu will only contain the message string*/
    size_t message_len = strlen(message);
    size_t padding_needed = 4 - message_len % 4;
    printf(" padding needed : %d ", padding_needed);
    size_t total_message_alloc_size= padding_needed + message_len;

    if(message_len%4==0){
        packet->message=calloc(total_message_alloc_size +1, sizeof(char));
        if(packet->message==NULL){
            perror("could not calloc sdu");
            exit(EXIT_FAILURE);
        }
         strcpy(packet->message, message);
         packet->message[message_len]= '\0';
        return 1;
    }
  
    packet->message=calloc(total_message_alloc_size +1, sizeof(char));
    if (packet->message==NULL){
        perror("sdu padding malloc failed");
        exit(EXIT_FAILURE);
    }
    strcpy(packet->message, message);
    //adding nulltrerminator to string 
    packet->message[message_len ] = '\0';
    printf("message length : %d", message_len);
    return total_message_alloc_size;
}



int main(int argc, char *argv[]){

    /*reading command line arguments, if len is 3 then -h option is not present */
    char *unix_socket_path ;
    uint8_t dst_mip_addr;
    uint8_t arg;
    uint8_t h_option ;
    struct mip_client_payload *packet = malloc(sizeof(struct mip_client_payload));
    size_t total_message_length_inc_padding ;
    if(packet==NULL){
        perror("could not malloc packet ");
        exit(EXIT_FAILURE);
    }
    if (argc == 4){
        unix_socket_path = argv[1];
        packet->dst_mip_addr = (uint8_t)atoi(argv[2]);
        total_message_length_inc_padding = fill_sdu(packet, argv[3]);
    
    }
    //h option flag is passed
    else{
        h_option = argv[1];
        unix_socket_path = argv[2];
        packet->dst_mip_addr = (uint8_t)atoi(argv[3]);
        total_message_length_inc_padding= fill_sdu(packet, argv[4]);
        
    }
    printf(" total message len : %d \n" , total_message_length_inc_padding);
    struct sockaddr_un *address= malloc(sizeof(struct sockaddr_un));
    if (address==NULL){
        perror("could not malloc address in client");
        exit(EXIT_FAILURE);
    }
    //testing that unix socket is porperly set up
    int unix_data_socket = setupUnixSocket(unix_socket_path, address);
    size_t packet_size = sizeof(uint8_t)+ total_message_length_inc_padding;

    unixSocket_connect(unix_data_socket, unix_socket_path, address);
    unixSocket_send(unix_data_socket, packet, packet_size );
    printf("message %s sendt \n", packet->message);
    printf("dst address : %u ", packet->dst_mip_addr);
    printf("packet length : %d bytes", packet_size);
    close(unix_data_socket);
    
    free(address);
    free(packet->message);
    free(packet);

    exit(0);

}
