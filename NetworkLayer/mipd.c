#define ETH_P_MIP 0x88B5

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
#include "routingTable.h"//i still need this in order to get the create router request struct. mipd already contains too much code as is.

/*Notes: this application was developed with the intention of only having one active client and server at a time on each mipd. 
The intetion is that a client should be started only when there is no active. This has not been implemented here. 
The testing script launches multiple clients at the same time. Therefore not all of them will receive a PONG ACK every time the program is run.
However as long as only one server is running per mipd, the server should be able to receive all of the pings and send back pongs regardless of how many clients run at the same time.
This will in the testingscript only affect the clients getting the PONGs.

Another note. The buffers are not properly managed yet, hopefully i can fix this within the delivery of home exam 2.
I tested this on nrec and it should work.*/

int forward_engine(){
    return 0;
}

struct RoutingResponse *send_to_router_and_receive_response(char *sock_path, char* payload){
    //todo add timeout to waiting for response
    char *routerPath = malloc(strlen(sock_path) + strlen("_routingd") + 1); // +1 for null terminator
    strcpy(routerPath,sock_path);
    strcat(routerPath, "_routingd");
    struct sockaddr_un *addr=malloc(sizeof(struct sockaddr_un));
    int sock= setupUnixSocket(sock_path, addr);
    unixSocket_connect(sock,routerPath,addr);
    write(sock,payload,strlen(payload));
    //wait for response, have a timeout if none received
    //todo, add timeout
    char *response_serialized = malloc(sizeof(struct RoutingResponse));
    read(sock,response_serialized,sizeof(struct RoutingResponse));
    struct RoutingResponse * response_deserialized = deserialize_response(response_serialized);
    close(sock);
    free(routerPath);
    free(addr);
    return response_deserialized;
}

//function for serializing pdu to be sent over raw sockets
size_t serialize_pdu(struct mip_pdu *mip_pdu, char* buffer){

    size_t offset = 0;
    // Serialize the mip_header
    buffer[offset++] = mip_pdu->mip_header.dest_addr;  // 1 byte for dest_addr
    buffer[offset++] = mip_pdu->mip_header.src_addr;   // 1 byte for src_addr

    // Doing micro management of bits
    // Pack ttl (4 bits), sdu_len (9 bits), and sdu_type (3 bits) into 3 bytes
    uint16_t ttl_sdu_len = (mip_pdu->mip_header.ttl << 12) |
                           (mip_pdu->mip_header.sdu_len & 0x1FF);  // Pack ttl and sdu_len

    buffer[offset++] = (ttl_sdu_len >> 8) & 0xFF;  // High byte of ttl_sdu_len
    buffer[offset++] = ttl_sdu_len & 0xFF;         // Low byte of ttl_sdu_len
    buffer[offset++] = (mip_pdu->mip_header.sdu_type & 0x07);  // 3 bits for sdu_type

    // Now serialize the sdu 
    if (mip_pdu->mip_header.sdu_type == MIP_ARP) {
        // serialize ARP message
        memcpy(buffer + offset, mip_pdu->sdu.arp_msg_payload, ARP_SDU_SIZE);
        offset += ARP_SDU_SIZE;
    } else  {
        // Serialize regular message payload
        memcpy(buffer + offset, mip_pdu->sdu.message_payload, mip_pdu->mip_header.sdu_len);
        offset += mip_pdu->mip_header.sdu_len;
    }

    return offset;  // Return the total number of bytes serialized into the buffer
}
//function for deserializing pdu to be sent over raw sockets
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

    // Deserialize the sdu
    if (mip_pdu->mip_header.sdu_type == MIP_ARP) {
        // Deserialize ARP message
        mip_pdu->sdu.arp_msg_payload = (struct mip_arp_message*)malloc(ARP_SDU_SIZE);
        memcpy(mip_pdu->sdu.arp_msg_payload, buffer + offset, ARP_SDU_SIZE);
        offset += ARP_SDU_SIZE;
    } else {
        // Deserialize regular message payload
        mip_pdu->sdu.message_payload = (char*)malloc(mip_pdu->mip_header.sdu_len);
        memcpy(mip_pdu->sdu.message_payload, buffer + offset, mip_pdu->mip_header.sdu_len);
        offset += mip_pdu->mip_header.sdu_len;
    }

    return mip_pdu;
}
struct mip_pdu* create_mip_pdu( uint8_t sdu_type, uint8_t arp_type, uint8_t dst_mip_addr, char *message, uint8_t src_address, uint8_t ttl){

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
        header->ttl= ttl; //recommended to set to one according to exam text, not yet decreased when reaching hosts
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

        //fill mip arp message:
        mip_pdu->sdu.arp_msg_payload->type = arp_type; //request or response
        mip_pdu->sdu.arp_msg_payload->address = dst_mip_addr;//address to look up 
        mip_pdu->sdu.arp_msg_payload->padding = 0; //padd, one 0 will take the rest of the space so that the struct is in total 32 bits

        return mip_pdu; 
    }
    else {
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
            perror("client packet is null \n ");
            exit(EXIT_FAILURE);
        }

        header->dest_addr = dst_mip_addr;
        header->src_addr = src_address;
        header->ttl= ttl; //recommended to set to one according to exam text
        header->sdu_type= sdu_type; //can be ping or arp, in this case it is arp
        header->sdu_len = strlen(message); //size of message, since we have PING type here

        //filling the pdu
        struct mip_pdu *mip_pdu = malloc(sizeof(struct mip_pdu));
        memcpy(&mip_pdu->mip_header, header, sizeof(struct mip_header));

        mip_pdu->sdu.message_payload = malloc(strlen(message)+1);
        memcpy(mip_pdu->sdu.message_payload, message, strlen(message)+1);
    
        return mip_pdu; 
    }

    return NULL; //180

}
int handle_arp(struct mip_pdu *mip_pdu, uint8_t *src_mac, struct cache *cache, uint8_t self_mip_addr, int raw_socket,struct sockaddr_ll *socket_name){
    uint8_t src_mac_addr[6] ;
    memcpy(src_mac_addr,src_mac,6);
       if (mip_pdu->sdu.arp_msg_payload->type == REQUEST) {
            printf(" \n ================================= \n");
            printf("received ARP REQUEST from :");
            print_mac_addr(src_mac_addr, 6);

            uint8_t requested_address = mip_pdu->sdu.arp_msg_payload->address;
            if (requested_address == self_mip_addr) {
                printf("    -the arp wants our address \n ");
                
                send_arp_response(raw_socket, mip_pdu, 164, src_mac_addr, self_mip_addr, socket_name);
                // Add the sender to the cache
                add_to_cache(cache, mip_pdu->mip_header.src_addr, src_mac_addr, socket_name);
                printf("    -added entry to cache \n");
            } else {
                printf("not our address \n");
            }
            printf(" \n ================================= \n");
        }
        // RESPONSE PDU
        else if (mip_pdu->sdu.arp_msg_payload->type == RESPONSE) {
            printf(" \n ================================= \n");
            printf("received ARP RESPONSE from :");
            print_mac_addr(src_mac_addr, 6);

            // Add the sender's MAC to the cache
            add_to_cache(cache, mip_pdu->mip_header.src_addr, src_mac_addr, socket_name);
            printf("added entry to cache \n");

            // Fetch queued PDUs for this MIP address and send them
            struct mip_pdu *unsent_pdu;
            while ((unsent_pdu = fetch_queued_pdu_in_cache(cache, mip_pdu->mip_header.src_addr)) != NULL) {// we need to query router and send to necxt hop

                printf(" Unsent pdu message: %s \n", unsent_pdu->sdu.message_payload);
                if (unsent_pdu->mip_header.sdu_type != PING) {
                    perror("unsent pdu not ping pdu");
                    return -1;
                }

                send_raw_packet(raw_socket, unsent_pdu, src_mac_addr, socket_name);
                printf(" Unsent pdu that was on hold is now sent \n");
                free(unsent_pdu->sdu.message_payload); 
                free(unsent_pdu);  
            }
            printf(" \n ================================= \n");
        }
}
int handle_ping(struct mip_pdu *mip_pdu, uint8_t *src_mac, struct cache *cache, uint8_t self_mip_addr, int raw_socket,struct sockaddr_ll *socket_name, char *socketPath){
    uint8_t src_mac_addr[6] ;
    memcpy(src_mac_addr,src_mac,6);
    // IF THE PING MESSAGE IS PONG
        if (strncmp(mip_pdu->sdu.message_payload, "PONG:", 5) == 0) {
            printf("Received PONG message, forwarding it to client app. \n");

            // Allocate sockaddr_un and recv_sock_path for Unix socket communication
            struct sockaddr_un *address = malloc(sizeof(struct sockaddr_un));
            if (address == NULL) {
                perror("could not malloc address in client");
                exit(EXIT_FAILURE);
            }
            char *recv_sock_path = malloc(12); // usockAclient
            memcpy(recv_sock_path, socketPath, 6);
            memcpy(recv_sock_path + 6, "client\0", 7);
            printf("SOCKET PATH %s", recv_sock_path);

            // Set up Unix socket and send PONG to the client application
            int unix_data_socket = setupUnixSocket(recv_sock_path, address);
            int status = unixSocket_connect(unix_data_socket, recv_sock_path, address);
            if (status == -1) {
                printf("Could not connect to client application Unix socket. In MIP, PDUs are delivered in a best-effort, unreliable manner \n");
                printf("The problem could be due to no client bound to Unix socket at the time of transfer.\n");
                free(address);
                free(recv_sock_path);
                return 1;
            }
            printf("\n-----------sending to client app---------------\n");

            status = write(unix_data_socket, mip_pdu->sdu.message_payload, strlen(mip_pdu->sdu.message_payload));
            if (status < 0) {
                printf("Could not send to client application. In MIP, PDUs are delivered in a best-effort, unreliable manner \n");
                printf("The problem could be due to no client bound to Unix socket at the time of transfer.\n");
                close(unix_data_socket);
                free(address);
                free(recv_sock_path);
                return 1;
            }
    
            // Read ACK from client application
            char *recv_buff = malloc(5);
            status = read(unix_data_socket, recv_buff, 5);
            printf("RECEIVED ACK FROM CLIENT APP: %s \n", recv_buff);
            printf(" \n ================================= \n");

            close(unix_data_socket);
            free(recv_buff);
            free(recv_sock_path);
            free(address);

            return 1; // Return to avoid infinite loop
        } 
        else { // If the message is PING
            // Send PONG back to the sender
            size_t message_len = strlen(mip_pdu->sdu.message_payload);  // Get the length of the message
            size_t buffer_size = sizeof(uint8_t) + message_len + 1;     // +1 for null terminator

            // Dynamically allocate buffer for PONG message
            struct mip_client_payload *buffer_to_server = calloc(1, buffer_size);
            if (buffer_to_server == NULL) {
                perror("malloc for buffer_to_server failed");
                return -1;
            }

            // Copy source address and message to buffer
            uint8_t src_addr = mip_pdu->mip_header.src_addr;
            buffer_to_server->dst_mip_addr = &src_addr;
            buffer_to_server->message = mip_pdu->sdu.message_payload;

            // Set up Unix socket to send PONG to server app
            struct sockaddr_un *address = malloc(sizeof(struct sockaddr_un));
            if (address == NULL) {
                perror("could not malloc address in client");
                free(buffer_to_server);
                exit(EXIT_FAILURE);
            }
            int unix_data_socket = setupUnixSocket(socketPath, address);
            unixSocket_connect(unix_data_socket, socketPath, address);
            printf("\n-----------sending to server app---------------\n");

            int status = unixSocket_send(unix_data_socket, buffer_to_server, buffer_size);
            if (status < 0) {
                perror("Could not send PONG to server");
            }
            
            // Read PONG response from server
            char *recv_buff = malloc(200);
            status = read(unix_data_socket, recv_buff, 200);
            printf("RECEIVED PONG FROM SERVER: %s \n", recv_buff + 1);
            close(unix_data_socket);
            // Send PONG response back to the original sender
            uint8_t dst_mip_addr = mip_pdu->mip_header.src_addr;
            uint8_t dst_mac_addr[6];
            memcpy(dst_mac_addr, src_mac_addr, 6);

            struct mip_pdu *pong_pdu = create_mip_pdu(PING, NULL, dst_mip_addr, recv_buff + 1, self_mip_addr,15);
            status = send_raw_packet(raw_socket, pong_pdu, dst_mac_addr, socket_name);
            if (status < 0) {
                perror("send pong message failed");
                free(buffer_to_server);
                free(recv_buff);
                free(pong_pdu->sdu.message_payload);
                free(pong_pdu);
                free(address);
                return -1;
            }
            printf("    -PONG message relayed to dst host \n");
            printf(" \n ================================= \n");

            // Free dynamically allocated memory
            free(buffer_to_server); 
            free(recv_buff);
            free(pong_pdu->sdu.message_payload);
            free(pong_pdu);
            free(address);
        }
}
int handle_router_package(struct mip_pdu *pdu ,int raw, uint8_t *src_mac, char* sock_path){
    uint8_t src_mac_addr[6];
    memcpy(src_mac_addr, src_mac,6);
    printf("handling router package\n");
    //check if Hello or update

    if (strncmp(pdu->sdu.message_payload, "HELLO", 5) == 0) {
        // Received a HELLO message, forwarding to routing daemon
        printf("received hello package\n");
        // Create the path for the routing daemon socket
        char *routerPath = malloc(strlen(sock_path) + strlen("_routingd") + 1); // +1 for null terminator
        if (!routerPath) {
            perror("malloc failed for routerPath");
            return -1; // Handle error
        }
        strcpy(routerPath, sock_path);
        strcat(routerPath, "_routingd");
        struct sockaddr_un *address = malloc(sizeof(struct sockaddr_un));
        if (!address) {
            perror("malloc failed for address");
            free(routerPath);
            return -1;
        }
        // Set up connection to the routing daemon
        int router_sock = setupUnixSocket(routerPath, address);
        int status = unixSocket_connect(router_sock, routerPath, address);
        if (status == -1) {
            perror("connect to router");
            free(routerPath);
            free(address);
            close(router_sock);
            return -1;
        }
        // Prepare payload, including the source address of the MIP node
        char *payload = malloc(strlen("HELLO") + 1 + sizeof(uint8_t)); // "HELLO" + 1 null byte + src_addr
        if (!payload) {
            perror("malloc failed for payload");
            free(routerPath);
            free(address);
            close(router_sock);
            return -1;
        }
        strcpy(payload, "HELLO");
        // Use a temporary variable for src_addr since it’s a bit-field
        uint8_t src_addr_value = pdu->mip_header.src_addr;
        memcpy(payload + strlen("HELLO"), &src_addr_value, sizeof(uint8_t));
        // Send payload to the routing daemon
        status = unixSocket_send_String(router_sock, payload, strlen("HELLO") + sizeof(uint8_t));
        if (status < 0) {
            perror("unixSocket_send_String");
        }

        // Cleanup
        close(router_sock);
        free(routerPath);
        free(address);
        free(payload);
    }

    else if(strncmp(pdu->sdu.message_payload, "UPPDATE", 7) == 0){
        //uppdate routes
        printf("received update package \n");
    }
    else{
        printf(" UNVALID ROUTER PACKAGE\n");
    }
    return 0;
}
// The main function for handling incomming raw socket connections. The packet types are checked and handled accordingly.
int serve_raw_connection(int raw_socket, struct sockaddr_ll *socket_name, uint8_t self_mip_addr, struct cache *cache, struct ifs_data* ifs, int unix_socket, char * socketPath) {
    uint8_t src_mac_addr[6];
    struct mip_pdu *mip_pdu = recv_pdu_from_raw(raw_socket, src_mac_addr);

    //  ARP PDU 
    if (mip_pdu->mip_header.sdu_type == MIP_ARP) {
        handle_arp(mip_pdu, src_mac_addr,cache,self_mip_addr,raw_socket,socket_name);
    } 
    else if (mip_pdu->mip_header.sdu_type == PING) {
        printf(" \n ================================= \n");
        printf("    -Received message from %d  \n", mip_pdu->mip_header.src_addr);
        printf("    -Message: %s \n", mip_pdu->sdu.message_payload);
        handle_ping(mip_pdu, src_mac_addr,cache,self_mip_addr,raw_socket,socket_name, socketPath);
    }
    else if (mip_pdu->mip_header.sdu_type == ROUTER){
        printf("received router package \n");
        handle_router_package(mip_pdu,raw_socket,src_mac_addr, socketPath );
        
    }
    return 1;
}

// Main function for handling incoming unix domain socket connections
int serve_unix_connection(int sock_accept, int raw_socket, struct cache *cache, uint8_t self_mip_addr, struct ifs_data *ifs, char * sock_path) {

    struct mip_client_payload *buffer = malloc(sizeof(struct mip_client_payload));
    char recv_buffer[1000];
    memset(recv_buffer, 0, sizeof(recv_buffer));
    printf(" \n ================================= \n");
    int bytes_read = read(sock_accept, recv_buffer, sizeof(recv_buffer));
    if (bytes_read ==-1 ) {
        close(sock_accept);
        perror("read unix sock");
        free(buffer);
        return -1;
    }
    close(sock_accept);
    printf("message read \n");
    /* checking if it is a roputer hello message, first byte from client will always be a mip address so this should work.*/
    if (strncmp(recv_buffer, "HELLO", 5) == 0){
        printf("\n ++++++Router wants to send HELLO messages to nearby hosts.++++++\n");
        //TODO create mip datagram to be sent to all neighbor hosts and wait for response and send back to router, then router will map neigbpurs
        struct mip_pdu * pdu= create_mip_pdu(ROUTER,NULL,255,"HELLO\0",self_mip_addr,1);//ttl must be one since we only want to reach nearby nodes.
        //send arp package over all interfaces to all conneted hosts
        send_broadcast_message(raw_socket,ifs,pdu);
        printf("HELLO broadcast message relayed\n");

        free(buffer);
        return 0;   
    }
    else if (strncmp(recv_buffer, "UPPDATE", 7) == 0){
        printf("\n ++++++Router wants to send UPPDATE messages to nearby hosts.++++++\n");
        free(buffer);

        return 0;
    }
    else{
        // else: here it will handle client or server messages
        size_t message_len = strlen(recv_buffer) - sizeof(uint8_t);
        buffer->message = malloc(message_len + 1);
        memset(buffer->message, 0, message_len + 1);
        memcpy(&(buffer->dst_mip_addr), recv_buffer, sizeof(uint8_t));
        memcpy(buffer->message, recv_buffer + sizeof(uint8_t), message_len);

        printf("    -dst mip address: %d  \n", buffer->dst_mip_addr);
        printf("    -message: %s  \n", buffer->message);
        //ask router for next hop to send the packet to 
        struct RoutingRequest RoutingRequest = create_routing_request(self_mip_addr,buffer->dst_mip_addr, 15 );
        //send to router and wait for response
        char* payload = serialize_router_requests(RoutingRequest);
        //wait for response, if none then end 
        struct RoutingResponse *response = send_to_router_and_receive_response(sock_path,payload);
        //TODO IMPLEMENT TIMEOUT IN SEND TO ROUTER AND RECEIVE RESPONSE, IF TIMED OUT RETURN -1, 

        //we have the response, get the next hop and send package to next hop, hopefully the next host will forward the package further
        uint8_t next_hop = response->next_hop_mip_addr;
        printf("///////////////// NEXT HOP ADDR: %d \n",next_hop);


        struct entry *cache_entry = get_mac_from_cache(cache, next_hop);// we will get the mac of next host and send it 

        if (cache_entry == NULL || cache_entry->mac_address == NULL || cache_entry->if_addr == NULL) {
            // ARP BROADCAST
            printf("MIP address not found in cache, sending ARP request \n");
            struct mip_pdu *ping_pdu_to_store = create_mip_pdu(PING, REQUEST, buffer->dst_mip_addr, buffer->message, self_mip_addr,15);
            struct mip_pdu *pdu = create_mip_pdu(MIP_ARP, REQUEST, next_hop, buffer->message, self_mip_addr,4);//requesting next hop, should be neibouhr so ttl is 1

            uint8_t broadcast_addr[] = BROADCAST_ADDRESS;

            // Send ARP over all interfaces
            for (int i = 0; i < ifs->ifn; i++) {
                printf("Sending ARP request on interface %d\n", i);
                send_raw_packet(ifs->rsock[i], pdu, broadcast_addr, &ifs->addr[i]);
            }

            // Legg PDU i kø i påvente av ARP-svar
            add_to_cache(cache, buffer->dst_mip_addr, NULL, NULL);
            add_pdu_to_queue(cache, buffer->dst_mip_addr, ping_pdu_to_store);
            printf("PDU is added in waiting queue, will be sent when response is received\n");

        } else {
            printf("MIP address found in cache, sending message\n");
            struct mip_pdu *pdu = create_mip_pdu(PING, REQUEST, buffer->dst_mip_addr, buffer->message, self_mip_addr,15);

            //TODO REQUEST ROUTE FROM ROUTER AND SEND PACKAGE TO NEXT HOP. THEN NEXT HOP WILL FORWARD IT FURTHER
 
            send_raw_packet(raw_socket, pdu, cache_entry->mac_address, cache_entry->if_addr);//sending raw paclet to next hop

        }

        free(buffer->message);
        free(buffer);
        printf(" \n ================================= \n");

        return 1;
    }

    return 0;

}

// Listeneing on sockets and handling events on them
void handle_events(int unix_socket, struct ifs_data *ifs, struct cache *cache, uint8_t self_mip_addr, char * socketPath) {
    int status, readyIOs;
    struct epoll_event event, events[10]; // 10 er maks antall hendelser
    int epoll_fd = epoll_create1(0);
    int sock_accept;
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // add unix socket to epoll
    add_to_epoll_table(epoll_fd, &event, unix_socket);

    // add raw socket to epoll
    for (int i = 0; i < ifs->ifn; i++) {
        add_to_epoll_table(epoll_fd, &event, ifs->rsock[i]);
    }

    while (1) {
        readyIOs = epoll_wait(epoll_fd, events, 10, -1);
        if (readyIOs == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < readyIOs; i++) {
            printf("\n ///WATING FOR PACKAGES///\n");
            if (events[i].data.fd == unix_socket) {
                int sock_accept = accept(unix_socket, NULL, NULL);
                if (sock_accept == -1) {
                    perror("accept");
                    continue;
                }
                serve_unix_connection(sock_accept, ifs->rsock[0], cache, self_mip_addr, ifs, socketPath);
                //close(sock_accept);
            } else {
                // Håndtere hendelser på raw sockets
                for (int j = 0; j < ifs->ifn; j++) {
                    if (events[i].data.fd == ifs->rsock[j]) {
                        serve_raw_connection(ifs->rsock[j], &(ifs->addr[j]), self_mip_addr, cache, ifs, unix_socket , socketPath);
                    }
                }
            }
        }
    }
}

// Main funciton, taking in parameters and starting the mip daemon
int main(int argc, char *argv[]) {
    uint8_t self_mip_addr;
    char *d_flag;
    char *socketPath;

    // handling arguments
    if (argc == 4) {
        d_flag = argv[1];
        socketPath = argv[2];
        self_mip_addr = (uint8_t)atoi(argv[3]);
    } else {
        socketPath = argv[1];
        self_mip_addr = (uint8_t)atoi(argv[2]);
    }

    printf("own mip addr : %d\n", self_mip_addr);
    printf("socket path : %s\n", socketPath);

    //setting up cache datastructures
    struct cache *cache = malloc(sizeof(struct cache));
    if (cache == NULL) {
        perror("could not malloc cache");
        exit(EXIT_FAILURE);
    }


    struct ifs_data *ifs = malloc(sizeof(struct ifs_data));
    if (ifs == NULL) {
        perror("could not malloc ifs_data");
        exit(EXIT_FAILURE);
    }
    // initialize interfaces
    init_ifs(ifs);

    // UNIX socket setup
    int unix_connection_socket, unix_data_socket;
    int status;
    struct sockaddr_un *address = malloc(sizeof(struct sockaddr_un));
    if (address == NULL) {
        perror("could not malloc address");
        exit(EXIT_FAILURE);
    }
   
    int mask = umask(0);
    unlink(socketPath); //remove socket if exists, only one can be bound to a socket
    unix_connection_socket = setupUnixSocket(socketPath, address);
    unix_data_socket = unixSocket_bind(unix_connection_socket, socketPath, address);
    status = unixSocket_listen(unix_connection_socket, NULL, unix_data_socket);
    if (status == -1) {
        perror("unixSocket_listen failed");
        exit(EXIT_FAILURE);
    }
    umask(mask);

    handle_events(unix_connection_socket, ifs, cache, self_mip_addr, socketPath);

    // cleaning and closing up
    for (int i = 0; i < ifs->ifn; i++) {
        close(ifs->rsock[i]);
    }
    close(unix_connection_socket);
    close(unix_data_socket);

    free(address);
    free(ifs);
    free(cache);


    return 0;
}
