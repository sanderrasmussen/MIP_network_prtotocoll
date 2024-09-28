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

/*  if(VARIBLE==-1){
        fprintf(stderr, "Error: XXXX");
        exit(-1);
    }
*/
int serve_connection(struct epoll_event *events,int sock_accept){
    /* an already existing client is trying to send packets*/
    char buffer[1024];
    int rc;
    //set all in buffer to 0
    memset(buffer, 0, sizeof(buffer));

    rc = read(events->data.fd, buffer, sizeof(buffer));
    if (rc <= 0) {
        close(events->data.fd);
        printf("<%d> left the chat...\n", events->data.fd);
        return -1;
    }
    printf("<%d>: %s\n", sock_accept, buffer);
    return 1;
};


void handle_events(int unix_connection_socket){
    int status;
    struct epoll_event event, events[10];//10 is max events
    int epoll_socket;
    int sock_accept;

    /* epoll file descriptor for event handlings */
	epoll_socket = epoll_create1(0);
	if (epoll_socket == -1) {
		perror("epoll_create1");
		close(unix_connection_socket );
		exit(EXIT_FAILURE);
	}

    //adding the connection coket to table 
    status= add_to_epoll_table(epoll_socket, &event, unix_connection_socket);
    if(status==-1){
        perror("add to epoll table failed");
        exit(EXIT_FAILURE);
    }
    
    for(;;){
        status= epoll_wait(epoll_socket, events, 10, -1);
        if (status==-1){
            perror("epoll wait error");
            close(unix_connection_socket);
            exit(EXIT_FAILURE);
        }
        /* If someone is trying to connect*/
        if (events->data.fd == unix_connection_socket){
            sock_accept= accept(events->data.fd,NULL,NULL);
            if (sock_accept== -1){
                perror("error on accept connection");
                continue;
            }

            printf("client connected");

            status = add_to_epoll_table(epoll_socket, &event , sock_accept);
            if (status== -1){
                close(unix_connection_socket);
                perror("add to epoll table error");
                exit(EXIT_FAILURE);
            }
        }
        else{
            serve_connection(events, sock_accept);
        }
    }
}
int main(int argc, char *argv[]){ 

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
    handle_events(unix_connection_socket);

    close(unix_connection_socket);
    close(unix_data_socket);
    unlink(pathToSocket);
    free(address);
    exit(1);
    return 1;
};

