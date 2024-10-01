
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



int main(int argc, char *argv[]){

    /*reading command line arguments, if len is 3 then -h option is not present */
    char *unix_socket_path ;
    uint8_t dst_mip_addr;
    char *message;
    uint8_t arg;
    uint8_t h_option ;
    struct mip_application_packet *packet = malloc(sizeof(struct mip_application_packet));

    if (argc == 4){
        unix_socket_path = argv[1];
        dst_mip_addr = argv[2];
        message = argv[3];
    }
    //h option flag is passed
    else{
        h_option = argv[1];
        unix_socket_path = argv[2];
        dst_mip_addr = argv[3];
        message = argv[4];
    }

    packet->dst_mip_addr=dst_mip_addr;
    packet->message = message;


    struct sockaddr_un *address= malloc(sizeof(struct sockaddr_un));
    //testing that unix socket is porperly set up
    int unix_data_socket = setupUnixSocket(unix_socket_path, address);
 
    unixSocket_connect(unix_data_socket, unix_socket_path, address);
    unixSocket_send(unix_data_socket, message);
    printf("message sendt");
    close(unix_data_socket);

    exit(0);

}

