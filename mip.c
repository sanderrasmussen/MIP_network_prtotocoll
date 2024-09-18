#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>


int main(){

    char *socket_path ="/unixSocket/unix.sock"; //remember to null terminate
    /* Creating Unic socket */
    struct sockaddr_un unix_socket_name;
    int unix_connection_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(unix_connection_socket==-1){
        fprintf(stderr, "Error: could not create unix socket");
        exit(-1);
    }

    /*
    * For portability we clear the  structure
    */
    memset(&unix_socket_name, 0, sizeof(unix_socket_name));   

    /* binding the unix socket to the name */
    unix_socket_name.sun_family= AF_UNIX;
    strcpy(unix_socket_name.sun_path, socket_path, sizeof(socket_path));

    int status = bind(unix_connection_socket, (const struct sockaddr *) &unix_socket_name, sizeof(unix_socket_name));
    if (status == -1){
        fprintf(stderr, "Error: could not bind unix socket");
        exit(1);
    }
    /* Listening with backlog of 0, meaning we will not accept any client to queue */
    int status = listen(unix_connection_socket,0);
    if (status==-1){
        fprintf(stderr, "Error: could not listen on unix connection socket");
        exit(-1);
    }


 /* Wait for incoming connection. */

    int data_socket = accept(unix_connection_socket, NULL, NULL);
    if (data_socket == -1) {
        perror("accept");
        exit(-1);
    }

    /* Close socket */
    close(data_socket);
    unlink(unix_socket_name);
    exit(1);
}

