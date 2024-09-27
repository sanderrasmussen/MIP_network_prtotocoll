
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

    
//struct sockaddr_un *address= malloc(sizeof(struct sockaddr_un));

int setupUnixSocket(char *pathToSocket, struct sockaddr_un *address){

    int status;
    int unix_sockfd;

     /* clearing structure */
    memset(address, 0 , sizeof(struct sockaddr_un));
    unlink(pathToSocket);

    /*create socket */
    unix_sockfd = socket(AF_UNIX, SOCK_STREAM,0);
    if(unix_sockfd== -1){
        fprintf(stderr, "Error: could not create socket ");
        close(unix_sockfd);
        unlink(pathToSocket);
        exit(EXIT_FAILURE);
    };
    return unix_sockfd;
}
/*server function*/
int unixSocket_bind(int unix_sockfd, char *pathToSocket, struct sockaddr_un *address){
    int status;
    int unix_data_socket;

      /*connect socket */
    address->sun_family=AF_UNIX;
    strncpy(address->sun_path, pathToSocket, sizeof(address->sun_path)-1);
    /* Using umask to give file permissions*/
    int mask =umask(pathToSocket);
    status = bind(unix_sockfd, (struct sockaddr *) &address, sizeof(struct sockaddr_un));
    if(status== -1){
        fprintf(stderr, "Error: could not bind socket ");
        close(unix_sockfd);
        unlink(pathToSocket);
        exit(EXIT_FAILURE);
    };
    umask(mask);
    return unix_data_socket;
}
//server function
int unixSocket_listen(int unix_sockfd, char *pathToSocket, int unix_data_socket){
    /* Opening listening for connections*/
    int status = listen(unix_sockfd,1);
    if(status== -1){
        fprintf(stderr, "Error: could not listen on socket ");
        close(unix_sockfd);
        unlink(pathToSocket);
        exit(EXIT_FAILURE);
    };

    for(;;){
        /* Wait for incoming connection. */
        unix_data_socket = accept(unix_sockfd, NULL, NULL);
        if (unix_data_socket == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("a message has been received ");
    };       
    close(unix_sockfd);
    unlink(pathToSocket);
    exit(1);
}
//client function
int unixSocket_connect(int unix_sockfd, char pathToSocket){
    struct sockaddr_un address;
    int status;
    int unix_data_socket;
    char buffer;

      /*connect socket */
    address.sun_family=AF_UNIX;
    strncpy(address.sun_path, pathToSocket, sizeof(address.sun_path)-1);
    /* Using umask to give file permissions*/
    int mask =umask(pathToSocket);
    status = connect(unix_sockfd, (struct sockaddr *) &address, sizeof(struct sockaddr_un));
    if(status== -1){
        fprintf(stderr, "Error: could not bind socket ");
        close(unix_sockfd);
        unlink(pathToSocket);
        exit(EXIT_FAILURE);
    };
    umask(mask);
    return unix_data_socket;
}
int unixSocket_send(int unix_data_socket, char* payload){
    int status = write(unix_data_socket,payload, sizeof(payload)+1);
    if (status == -1){
        perror("Error : unix socket write");
        exit(EXIT_FAILURE);
    }
}
int unixSocket_recieve(int unix_data_socket, char *buffer){
    int status= read(unix_data_socket,buffer, sizeof(buffer));
}
int close_unix_socket(int socket, char *socketname){
    close(socket);
    unlink(socketname);
}

