#define ETH_P_MIP 0x88B5
#define MIP_ADDRESS 20
#define MAX_MESSAGE_SIZE 1024
#define PDU_MESSAGE_BLOCK_SIZE 4 //reading 4 bytes/ 32bits at a time

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
#include "cache.h"


//raw mip ethernet communication

size_t serialize_pdu(struct mip_pdu *mip_pdu, char* buffer){
    //convert mip pdu into a buffer carray

    size_t offset = 0;

    // Serialize the mip_header
    buffer[offset++] = mip_pdu->mip_header.dest_addr;  // 1 byte for dest_addr
    buffer[offset++] = mip_pdu->mip_header.src_addr;   // 1 byte for src_addr

    printf("serialised dst address : %d" , mip_pdu->mip_header.dest_addr);
    printf("serialised dst address : %d" , buffer[0]  );
    // Pack ttl (4 bits), sdu_len (9 bits), and sdu_type (3 bits) into 3 bytes
    uint16_t ttl_sdu_len = (mip_pdu->mip_header.ttl << 12) |
                           (mip_pdu->mip_header.sdu_len & 0x1FF);  // Pack ttl and sdu_len

    buffer[offset++] = (ttl_sdu_len >> 8) & 0xFF;  // High byte of ttl_sdu_len
    buffer[offset++] = ttl_sdu_len & 0xFF;         // Low byte of ttl_sdu_len
    buffer[offset++] = (mip_pdu->mip_header.sdu_type & 0x07);  // 3 bits for sdu_type

    // Now serialize the sdu (payload)
    if (mip_pdu->mip_header.sdu_type == MIP_ARP) {
        // Serialize ARP message
        memcpy(buffer + offset, mip_pdu->sdu.arp_msg_payload, ARP_SDU_SIZE);
        offset += ARP_SDU_SIZE;
    } else if (mip_pdu->mip_header.sdu_type == PING) {
        // Serialize regular message payload
        memcpy(buffer + offset, mip_pdu->sdu.message_payload, mip_pdu->mip_header.sdu_len);
        offset += mip_pdu->mip_header.sdu_len;
    }

    //serialize test
//    / print("serialized message %s ", )
    return offset;  // Return the total number of bytes serialized into the buffer
}
struct mip_pdu* deserialize_pdu(char* buffer, size_t length) {
    struct mip_pdu* mip_pdu = (struct mip_pdu*)malloc(sizeof(struct mip_pdu));
    size_t offset = 0;

    // Deserialize the mip_header
    mip_pdu->mip_header.dest_addr = buffer[offset++];   // 1 byte for dest_addr
    mip_pdu->mip_header.src_addr = buffer[offset++];    // 1 byte for src_addr

    // Extract ttl (4 bits), sdu_len (9 bits), and sdu_type (3 bits)
    uint16_t ttl_sdu_len = (buffer[offset] << 8) | buffer[offset + 1];
    mip_pdu->mip_header.ttl = (ttl_sdu_len >> 12) & 0x0F;  // 4 bits for TTL
    mip_pdu->mip_header.sdu_len = ttl_sdu_len & 0x1FF;     // 9 bits for sdu_len
    mip_pdu->mip_header.sdu_type = buffer[offset + 2] & 0x07;  // 3 bits for sdu_type
    offset += 3;  // Move the offset by 3 bytes (2 for ttl_sdu_len and 1 for sdu_type)

    // Deserialize the sdu (payload)
    if (mip_pdu->mip_header.sdu_type == MIP_ARP) {
        // Deserialize ARP message
        mip_pdu->sdu.arp_msg_payload = (struct mip_arp_message*)malloc(ARP_SDU_SIZE);
        memcpy(mip_pdu->sdu.arp_msg_payload, buffer + offset, ARP_SDU_SIZE);
        offset += ARP_SDU_SIZE;
    } else if (mip_pdu->mip_header.sdu_type == PING) {
        // Deserialize regular message payload
        mip_pdu->sdu.message_payload = (char*)malloc(mip_pdu->mip_header.sdu_len);
        memcpy(mip_pdu->sdu.message_payload, buffer + offset, mip_pdu->mip_header.sdu_len);
        offset += mip_pdu->mip_header.sdu_len;
    }

    return mip_pdu;
}
struct mip_pdu* create_mip_pdu( uint8_t sdu_type, uint8_t arp_type, uint8_t dst_mip_addr, char *message, uint8_t src_address){

    if(sdu_type==MIP_ARP){

    /* HEADER: 
     +--------------+-------------+---------+-----------+-----------+
     | Dest. Addr.  | Src. Addr.  | TTL     | SDU Len.  | SDU type  |
     +--------------+-------------+---------+-----------+-----------+
     | 8 bits       | 8 bits      | 4 bits  | 9 bits    | 3 bits    |
     +--------------+-------------+---------+-----------+-----------+
     */
        struct mip_header *header = malloc(sizeof(struct mip_header));
        //filling the header values
        if (dst_mip_addr==NULL){
            perror("client packet is null ");
            exit(EXIT_FAILURE);
        }
        header->dest_addr = dst_mip_addr;
        header->src_addr = src_address;
        header->ttl= 1; //recommended to set to one according to exam text
        header->sdu_type= sdu_type; //can be ping or arp, in this case it is arp
        header->sdu_len = sizeof(uint32_t);
         /*if it is arp, then we know that the sdu len is only 32 bit because the payload is:
       
                    +-------+-------------------------+-----------------------------+
            | Type  | Address                 | Padding/ Reserved           |
            +-------+-------------------------+-----------------------------+
            | 0x00  | MIP address to look up  | Pad with 0x00 until 32 bits |
            +-------+-------------------------+-----------------------------+

        */

        //adding the payload 
        struct mip_pdu* mip_pdu = malloc(sizeof(struct mip_header)+ header->sdu_len*sizeof(char));
        mip_pdu->sdu.arp_msg_payload = malloc(sizeof(struct mip_arp_message));
        
        memcpy(&mip_pdu->mip_header, header, sizeof(struct mip_header));
        /*filling sdu which is pointer to
            struct mip_arp_message{
                uint32_t type : 1;
                uint32_t address : 8;
                uint32_t padding : 23;
        }__attribute__((packed));
        */
        //fill mip arp message:
        mip_pdu->sdu.arp_msg_payload->type = arp_type; //request or response
        mip_pdu->sdu.arp_msg_payload->address = dst_mip_addr;//address to look up 
        mip_pdu->sdu.arp_msg_payload->padding = 0; //padd, one 0 will take the rest of the space so that the struct is in total 32 bits

        return mip_pdu; 
    }
    else if(sdu_type==PING){
        //fill mip header
         /* HEADER: 
     +--------------+-------------+---------+-----------+-----------+
     | Dest. Addr.  | Src. Addr.  | TTL     | SDU Len.  | SDU type  |
     +--------------+-------------+---------+-----------+-----------+
     | 8 bits       | 8 bits      | 4 bits  | 9 bits    | 3 bits    |
     +--------------+-------------+---------+-----------+-----------+
     */
        struct mip_header *header = malloc(sizeof(struct mip_header));
        //filling the header values
        if (dst_mip_addr==NULL){
            perror("client packet is null ");
            exit(EXIT_FAILURE);
        }

        header->dest_addr = dst_mip_addr;
        header->src_addr = src_address;
        header->ttl= 1; //recommended to set to one according to exam text
        header->sdu_type= sdu_type; //can be ping or arp, in this case it is arp
        header->sdu_len = strlen(message); //size of message, since we have PING type here

        //filling the pdu
        struct mip_pdu *mip_pdu = malloc(sizeof(struct mip_pdu));
        memcpy(&mip_pdu->mip_header, header, sizeof(struct mip_header));

        mip_pdu->sdu.message_payload = malloc(strlen(message)+1);
        memcpy(mip_pdu->sdu.message_payload, message, strlen(message)+1);
        
        printf("pdu test : \n");
        printf("dst addr %d ", mip_pdu->mip_header.dest_addr);
    


        return mip_pdu; 
    }

    return NULL;

}
int serve_raw_connection( int raw_socket, struct sockaddr_ll *socket_name, uint8_t self_mip_addr, struct cache *cache){
    uint8_t src_mac_addr[6]; 
    struct mip_pdu * mip_pdu = recv_pdu_from_raw(raw_socket, src_mac_addr);
    //this is the interesting part, because now we need to answer if we have the mip address and send response back
    if(mip_pdu->mip_header.sdu_type==MIP_ARP){
        printf("received ARP from :");
        print_mac_addr(src_mac_addr, 6); 
        printf(" testes ");
        uint8_t requested_address = mip_pdu->sdu.arp_msg_payload->address;
        //printf(" , the requested address is %d \n ", requested_address);
        //printf(" our address is : %d \n ", self_mip_addr);
        if(requested_address == self_mip_addr){
            printf(" mac : ");
            print_mac_addr(src_mac_addr, 6);
            print(" wants our address \n "); 
            send_arp_response(raw_socket, socket_name, mip_pdu, 164, src_mac_addr, self_mip_addr); 
            // add the sender host to cache
            add_to_cache(cache, mip_pdu->mip_header.src_addr , src_mac_addr);
        }
    }
    else{
        printf("===Received message from %d === \n", mip_pdu->mip_header.src_addr);
        printf( "===message : %s === \n", mip_pdu->sdu.message_payload);
    }
    close(raw_socket);
    return 1;
   
}

int serve_unix_connection(struct epoll_event *events,int sock_accept, int raw_socket, struct sockaddr_ll *socket_name, struct cache *cache){
    /* an already existing client is trying to send packets*/
    struct mip_client_payload *buffer=malloc(sizeof(struct mip_client_payload));

    int rc;
    char recv_buffer[MAX_MESSAGE_SIZE];
    memset(buffer, 0, sizeof(struct mip_client_payload));
    memset(recv_buffer,0,sizeof(MAX_MESSAGE_SIZE + 1));
    int bytes_read = read(events->data.fd, recv_buffer, MAX_MESSAGE_SIZE);
    if (bytes_read == -1) {
    close(events->data.fd);
    printf("<%d> left the chat...\n", events->data.fd);
    return -1;
    }
    size_t message_len = strlen(recv_buffer) - sizeof(uint8_t);
    buffer->message= malloc(message_len+1);
    memset(buffer->message, 0, message_len+1);
    memcpy(&(buffer->dst_mip_addr), recv_buffer, sizeof(uint8_t));
    memcpy(buffer->message , recv_buffer + sizeof(uint8_t),message_len );
    printf("bytes read: %d \n" , bytes_read);
    printf("buffer content %s \n" , recv_buffer);
    printf("message size in bytes : %d", message_len);

    //unpack unix socket message
    if(buffer->dst_mip_addr==NULL){
        perror("dst addr mip message is not valid ");
        exit(EXIT_FAILURE);
    }
    printf("\n");
    printf("=== dst mip address: %d === \n", buffer->dst_mip_addr );
       //unpack unix socket message
    if(buffer->message==NULL){
        perror("MIP message is not valid ");
        exit(EXIT_FAILURE);
    }
    printf("\n");
    printf("=== message:%s === \n", buffer->message);

    //check cahce, if empty send arp
    struct entry *cache_entry= get_mac_from_cache(cache, buffer->dst_mip_addr);
    if (cache_entry==NULL){
        //create mipheader and packet 
        //struct mip_pdu *pdu = create_mip_datagram(buffer, MIP_ARP, REQUEST);
        struct mip_pdu *pdu = create_mip_pdu( MIP_ARP, REQUEST, buffer->dst_mip_addr, buffer->message, buffer->dst_mip_addr);
        //printf("pdu message %s \n",  pdu->sdu.message_payload);
        send_arp(raw_socket,socket_name,   pdu);
    }
    //if mac in cache we just send the ping directly
    else{
        //TO BE IMPLEMENTED 
        //send_raw_packet();
    }
     

    printf("\n");
    free(buffer->message);
    free(buffer);
    close(events->data.fd);
 
    return 1;
};


void handle_events(int socket,int raw_socket, struct sockaddr_ll *socket_name, struct cache *cache, uint8_t self_mip_addr){
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
                serve_raw_connection(events[i].data.fd, socket_name, self_mip_addr, cache );
            }
            //someone is connected to unix socket and wants to send data
            else{
                serve_unix_connection(events, sock_accept, raw_socket, socket_name, cache);
            }
        }
    }
}


int main(int argc, char *argv[]){ 
    uint8_t self_mip_addr=20;
    
    struct cache* cache= malloc(sizeof(struct cache));
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

    handle_events(unix_connection_socket,raw_socket, socket_name, cache, self_mip_addr);

    close(raw_socket);
    close(unix_connection_socket);
    close(unix_data_socket);
    unlink(pathToSocket);
    free(address);
    exit(1);
    return 1;
};