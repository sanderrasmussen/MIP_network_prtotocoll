

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/stat.h>


/*  if(VARIBLE==-1){
        fprintf(stderr, "Error: XXXX");
        exit(-1);
    }
*/

int main(){

    /* global variables */
    struct sockaddr_un address;
    int status;
    int unix_sockfd;
    int unix_data_socket;
    char buffer;
    char *pathToSocket = "/unixSocket/unix.sock";

    /* clearing structure */
    memset(&address, 0 , sizeof(struct sockaddr_un));
    unlink(pathToSocket);

    /*create socket */
    int unix_sockfd = socket(AF_UNIX, SOCK_STREAM,0);
    if(unix_sockfd= -1){
        fprint(stderr, "Error: could not create socket ");
        exit(-1);
    };

    /*connect socket */
    address.sun_family=AF_UNIX;
    strncpy(address.sun_path, pathToSocket, sizeof(address.sun_path)-1);
    /* Using umask to give file permissions*/
    int mask =umask(pathToSocket);
    status = bind(unix_sockfd, (struct sockaddr *) &address, sizeof(struct sockaddr_un));
    if(status= -1){
        fprint(stderr, "Error: could not bind socket ");
        exit(-1);
    };
    umask(mask);

    /* Opening listening for connections*/
    status = listen(unix_sockfd,0);
    if(status= -1){
        fprint(stderr, "Error: could not listen on socket ");
        exit(-1);
    };

    for(;;){
        break;
    }
    close(unix_sockfd);
    unlink(pathToSocket);
    exit(1);

}


  