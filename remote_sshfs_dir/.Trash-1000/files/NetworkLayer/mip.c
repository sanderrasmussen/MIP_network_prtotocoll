#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/if_packet.h>
#include <netinet/ether.h>

/*  if(VARIBLE==-1){
        fprintf(stderr, "Error: XXXX");
        exit(-1);
    }
*/

int main(){

    
    char *pathToSocket = "/tmp/unix.sock";
    setupUnixSocket(pathToSocket);
    exit(1);

}


int setupUnixSocket(char *pathToSocket){
    
    struct sockaddr_un address;
    int status;
    int unix_sockfd;
    int unix_data_socket;
    char buffer;

     /* clearing structure */
    memset(&address, 0 , sizeof(struct sockaddr_un));
    unlink(pathToSocket);

    /*create socket */
    unix_sockfd = socket(AF_UNIX, SOCK_STREAM,0);
    if(unix_sockfd== -1){
        fprintf(stderr, "Error: could not create unix socket ");
        close(unix_sockfd);
        unlink(pathToSocket);
        exit(EXIT_FAILURE);
    };

    /*bind socket */
    address.sun_family=AF_UNIX;
    strncpy(address.sun_path, pathToSocket, sizeof(address.sun_path)-1);
    /* Using umask to give file permissions*/
    int mask =umask(pathToSocket);
    status = bind(unix_sockfd, (struct sockaddr *) &address, sizeof(struct sockaddr_un));
    if(status== -1){
        fprintf(stderr, "Error: could not bind unix socket ");
        close(unix_sockfd);
        unlink(pathToSocket);
        exit(EXIT_FAILURE);
    };
    umask(mask);

    /* Opening listening for connections*/
    status = listen(unix_sockfd,1);
    if(status== -1){
        fprintf(stderr, "Error: could not listen on unix socket ");
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

    }
       
    close(unix_sockfd);
    unlink(pathToSocket);
    exit(1);
}
int setupRawSocket(){
    int status;
    int raw_sockfd=(AF_INET, SOCK_RAW, htons(ETH_P_ALL)); //all protocols
    if(raw_sockfd == -1){
        fprintf(stderr, "Error: could not create raw socket ");
        close(raw_sockfd);
        exit(EXIT_FAILURE);
    }

    /* bind raw socket */
    //status = bind(raw_sockfd,)

}
/* The main loop where the mip will do its magic with unix and raw socket*/
int listenForConnections();