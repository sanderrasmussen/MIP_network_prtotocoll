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


//raw mip ethernet communication

size_t serialize_pdu(struct mip_pdu *mip_pdu, char* buffer){
    //convert mip pdu into a buffer carray

    size_t offset = 0;

    // Serialize the mip_header
    buffer[offset++] = mip_pdu->mip_header.dest_addr;  // 1 byte for dest_addr
    buffer[offset++] = mip_pdu->mip_header.src_addr;   // 1 byte for src_addr

    //printf("serialised dst address : %d" , mip_pdu->mip_header.dest_addr);
    //printf("serialised dst address : %d" , buffer[0]  );
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
            perror("client packet is null \n ");
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
        
        //printf("pdu test : \n");
        //printf("dst addr %d ", mip_pdu->mip_header.dest_addr);
    


        return mip_pdu; 
    }

    return NULL;

}
int serve_raw_connection(int raw_socket, struct sockaddr_ll *socket_name, uint8_t self_mip_addr, struct cache *cache, struct ifs_data* ifs, int unix_socket, char * socketPath) {
    uint8_t src_mac_addr[6];
    struct mip_pdu *mip_pdu = recv_pdu_from_raw(raw_socket, src_mac_addr);

    // Dette er den interessante delen der vi svarer på ARP eller sender meldinger tilbake
    if (mip_pdu->mip_header.sdu_type == MIP_ARP) {
        if (mip_pdu->sdu.arp_msg_payload->type == REQUEST) {
            printf(" \n ================================= \n");
            printf("received ARP REQUEST from :");
            print_mac_addr(src_mac_addr, 6);

            uint8_t requested_address = mip_pdu->sdu.arp_msg_payload->address;
            if (requested_address == self_mip_addr) {
                printf("    -the arp wants our address \n ");
                
                send_arp_response(raw_socket, mip_pdu, 164, src_mac_addr, self_mip_addr, socket_name);
                // Legg til senderen i cachen
      
                add_to_cache(cache, mip_pdu->mip_header.src_addr, src_mac_addr, socket_name);
                printf("    -added entry to cache \n");
            } else {
                printf("not our address \n");
            }
            printf(" \n ================================= \n");
        }

        else if (mip_pdu->sdu.arp_msg_payload->type == RESPONSE) {
            printf(" \n ================================= \n");
            printf("received ARP RESPONSE from :");
            print_mac_addr(src_mac_addr, 6);
            add_to_cache(cache, mip_pdu->mip_header.src_addr, src_mac_addr, socket_name);
            printf("added entry to cache \n");

            struct mip_pdu *unsent_pdu = fetch_queued_pdu_in_cache(cache, mip_pdu->mip_header.src_addr);
            if (unsent_pdu == NULL) {
                printf("could not fetch unsent pdu \n ");
                return -1;
            }
            printf(" Unsent pdu message : %s \n", unsent_pdu->sdu.message_payload);
            if(unsent_pdu->mip_header.sdu_type!=PING){
                perror("unsent pdu not ping pdu ");
                return -1;
            }
            send_raw_packet(raw_socket, unsent_pdu, src_mac_addr, socket_name);
            printf(" unsent pdu that was on hold is now sent \n");
            printf(" \n ================================= \n");
        }
    }

    else {
    printf(" \n ================================= \n");
    printf("    -Received message from %d  \n", mip_pdu->mip_header.src_addr);
    printf("    -message : %s \n", mip_pdu->sdu.message_payload);


    // IF THE PING MESSAGE IS PONG
    if (strncmp(mip_pdu->sdu.message_payload, "PONG:", 5) == 0) {
        printf("Received PONG message, forwarding it to client app. \n");
        struct sockaddr_un *address= malloc(sizeof(struct sockaddr_un));
        if (address==NULL){
            perror("could not malloc address in client");
            exit(EXIT_FAILURE);
        }
        //testing that unix socket is porperly set up
        char* recv_sock_path = malloc(12); //usockAclient
        memcpy(recv_sock_path, socketPath, 6);
        memcpy(recv_sock_path+6, "client\0",7);
        printf("SOCKET PATH %s", recv_sock_path);
        int unix_data_socket = setupUnixSocket(recv_sock_path, address);

        int status = unixSocket_connect(unix_data_socket, recv_sock_path, address);
        if(status==-1){
            printf("Could not connect to client application unix socket. In MIP, PDUs are delivered in a best effort, unreliable manner \n");
            printf("The problem could be due to no client bound to unix socket at the time of transfer.\n");       
            return 1;
        }
        printf("\n-----------sending to client app---------------\n");
        status = write(unix_data_socket, mip_pdu->sdu.message_payload, 100);
        if(status<0){
            printf("Could not send to client application. In MIP, PDUs are delivered in a best effort, unreliable manner \n");
            printf("The problem could be due to no client bound to unix socket at the time of transfer.\n");       
            return 1;
        }
        //printf("message %s sendt \n", packet->message);
        struct mip_client_payload *payload = malloc(200);
        char* recv_buff = malloc(200);
        status= read(unix_data_socket,recv_buff, 200);
        printf("RECEIVED ACK FROM CLIENT APP : %s \n",recv_buff+1 );
        printf(" \n ================================= \n");
        close(unix_data_socket);
        return 1;  // Returner tidlig for å unngå uendelig løkke
    }
    else{//if message is PING
        // Beregn korrekt størrelse på bufferet basert på størrelsen på meldingen
        size_t message_len = strlen(mip_pdu->sdu.message_payload);  // Finn lengden på meldingen
        size_t buffer_size = sizeof(uint8_t) + message_len + 1;     // +1 for null-terminator

        // Alloker dynamisk buffer for å holde dst_mip_addr og meldingen
        struct mip_client_payload *buffer_to_server = calloc(1, buffer_size);

        if (buffer_to_server == NULL) {
            perror("malloc for buffer_to_server failed");
            
            return -1;
        }

        // Kopier src_addr til en midlertidig variabel
        uint8_t src_addr = mip_pdu->mip_header.src_addr;

        // Kopier src_addr til bufferet
        buffer_to_server->dst_mip_addr= &src_addr;

        // Kopier meldingen etter src_addr
        buffer_to_server->message=  mip_pdu->sdu.message_payload;  // Inkluder null-terminator
        
    
        struct sockaddr_un *address= malloc(sizeof(struct sockaddr_un));
        if (address==NULL){
            perror("could not malloc address in client");
            exit(EXIT_FAILURE);
        }
        //testing that unix socket is porperly set up
        int unix_data_socket = setupUnixSocket(socketPath, address);


        unixSocket_connect(unix_data_socket, socketPath, address);
        printf("\n-----------sending to server app---------------\n");
        int status = unixSocket_send(unix_data_socket, buffer_to_server, buffer_size);
        //printf("message %s sendt \n", packet->message);
        struct mip_client_payload *payload = malloc(200);
        char* recv_buff = malloc(200);
        status= read(unix_data_socket,recv_buff, 200);
        printf("RECEIVED PONG FROM SERVER : %s \n",recv_buff+1 );

        uint8_t dst_mip_addr = mip_pdu->mip_header.src_addr;
        uint8_t dst_mac_addr[6] ;
        memcpy(dst_mac_addr, src_mac_addr, 6);
        printf("adfsadfs \n ");
        struct mip_pdu *pong_pdu =create_mip_pdu(PING,NULL,dst_mip_addr,recv_buff+1, self_mip_addr);

        status = send_raw_packet(raw_socket, pong_pdu, dst_mac_addr,socket_name);
        if (status <0){
            perror("send pong message \n");
            return -1;
        }
        printf("    -pong message releayed to dst host \n");
        printf(" \n ================================= \n");

        // Frigjør bufferet etter bruk
        free(buffer_to_server);
        close(unix_data_socket);
    }
}

    return 1;
}
int serve_unix_connection(int sock_accept, int raw_socket, struct cache *cache, uint8_t self_mip_addr, struct ifs_data *ifs) {
    /* Håndterer eksisterende klient som sender pakker */
    struct mip_client_payload *buffer = malloc(sizeof(struct mip_client_payload));
    char recv_buffer[1000];
    memset(recv_buffer, 0, sizeof(recv_buffer));
    printf(" \n ================================= \n");
    int bytes_read = read(sock_accept, recv_buffer, sizeof(recv_buffer));
    if (bytes_read == -1) {
        close(sock_accept);
        perror("read unix sock");
        return -1;
    }

    size_t message_len = strlen(recv_buffer) - sizeof(uint8_t);
    buffer->message = malloc(message_len + 1);
    memset(buffer->message, 0, message_len + 1);
    memcpy(&(buffer->dst_mip_addr), recv_buffer, sizeof(uint8_t));
    memcpy(buffer->message, recv_buffer + sizeof(uint8_t), message_len);

    printf("    -dst mip address: %d  \n", buffer->dst_mip_addr);
    printf("    -message: %s  \n", buffer->message);

    struct entry *cache_entry = get_mac_from_cache(cache, buffer->dst_mip_addr);
    if (cache_entry == NULL || cache_entry->mac_address == NULL || cache_entry->if_addr == NULL) {
        // ARP BROADCAST
        printf("MIP address not found in cache, sending ARP request \n");
        struct mip_pdu *ping_pdu_to_store = create_mip_pdu(PING, REQUEST, buffer->dst_mip_addr, buffer->message, self_mip_addr);
        struct mip_pdu *pdu = create_mip_pdu(MIP_ARP, REQUEST, buffer->dst_mip_addr, buffer->message, self_mip_addr);
        uint8_t broadcast_addr[] = BROADCAST_ADDRESS;

        // Send ARP over all interfaces
        for (int i = 0; i < ifs->ifn; i++) {
            printf("Sending ARP request on interface %d\n", i);
            send_raw_packet(ifs->rsock[i], pdu, broadcast_addr, &ifs->addr[i]);  // Bruk den aktuelle socketen og sockaddr_ll
        }

        // Legg PDU i kø i påvente av ARP-svar
        add_to_cache(cache, buffer->dst_mip_addr, NULL, NULL);
        add_pdu_to_queue(cache, buffer->dst_mip_addr, ping_pdu_to_store);
        printf("PDU is added in waiting queue, will be sent when response is received\n");

    } else {
        printf("MIP address found in cache, sending message\n");
        struct mip_pdu *pdu = create_mip_pdu(PING, REQUEST, buffer->dst_mip_addr, buffer->message, self_mip_addr);
        send_raw_packet(raw_socket, pdu, cache_entry->mac_address, cache_entry->if_addr);
    }
    printf(" \n ================================= \n");
    
    return 1;
}


void handle_events(int unix_socket, struct ifs_data *ifs, struct cache *cache, uint8_t self_mip_addr, char * socketPath) {
    int status, readyIOs;
    struct epoll_event event, events[10]; // 10 er maks antall hendelser
    int epoll_fd = epoll_create1(0);
    int sock_accept;
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // Legg til unix_socket i epoll
    add_to_epoll_table(epoll_fd, &event, unix_socket);

    // Legg til alle raw sockets i epoll
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
            if (events[i].data.fd == unix_socket) {
                int sock_accept = accept(unix_socket, NULL, NULL);
                if (sock_accept == -1) {
                    perror("accept");
                    continue;
                }
                serve_unix_connection(sock_accept, ifs->rsock[0], cache, self_mip_addr, ifs);
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


int main(int argc, char *argv[]) {
    uint8_t self_mip_addr;
    char *d_flag;
    char *socketPath;

    // Argumentbehandling
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

    // Allokere cache
    struct cache *cache = malloc(sizeof(struct cache));
    if (cache == NULL) {
        perror("could not malloc cache");
        exit(EXIT_FAILURE);
    }

    // Allokere ifs_data-strukturen for grensesnittene
    struct ifs_data *ifs = malloc(sizeof(struct ifs_data));

    if (ifs == NULL) {
        perror("could not malloc ifs_data");
        exit(EXIT_FAILURE);
    }

    // Initialisere ifs_data-strukturen med grensesnittinformasjon og raw sockets
    init_ifs(ifs);

    // UNIX socket setup
    int unix_connection_socket, unix_data_socket;
    int status;
    struct sockaddr_un *address = malloc(sizeof(struct sockaddr_un));
    if (address == NULL) {
        perror("could not malloc address");
        exit(EXIT_FAILURE);
    }

    unlink(socketPath); // Fjerne eventuell eksisterende socket
    unix_connection_socket = setupUnixSocket(socketPath, address);
    unix_data_socket = unixSocket_bind(unix_connection_socket, socketPath, address);
    status = unixSocket_listen(unix_connection_socket, NULL, unix_data_socket);
    if (status == -1) {
        perror("unixSocket_listen failed");
        exit(EXIT_FAILURE);
    }

    // Håndtere hendelser (både raw sockets og UNIX sockets)
    handle_events(unix_connection_socket, ifs, cache, self_mip_addr, socketPath);

    // Rydde opp og lukke sockets
    for (int i = 0; i < ifs->ifn; i++) {
        close(ifs->rsock[i]);
    }
    close(unix_connection_socket);
    close(unix_data_socket);
    unlink(socketPath);
    free(address);
    free(ifs);
    free(cache);

    return 0;
}
