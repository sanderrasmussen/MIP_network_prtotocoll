
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

int setupUnixSocket(char *pathToSocket, struct sockaddr_un *address);
/*server function*/
int unixSocket_bind(int unix_sockfd, char *pathToSocket, struct sockaddr_un *address);
//server function
int unixSocket_listen(int unix_sockfd, char *pathToSocket, int unix_data_socket);
int unixSocket_connect(int unix_sockfd, char *pathToSocket);
int unixSocket_send(int unix_data_socket, char* payload);
int unixSocket_recieve(int unix_data_socket, char *buffer);
int close_unix_socket(int socket, char *socketname);

