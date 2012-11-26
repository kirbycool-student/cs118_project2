#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <string.h>

#define DATAGRAM_SIZE 1000

void error(char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sock, port, nbytes;
    struct sockaddr_in serv_addr;

    char buffer[DATAGRAM_SIZE];

    if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) 
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    port = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    while(1) {

        //wait for a packet
        int size = sizeof(serv_addr);
        if( nbytes = recvfrom(sock, buffer, DATAGRAM_SIZE, 0, 
                                (struct sockaddr *) &serv_addr,
                                 &size) < 0)
        {
            error("recvfrom failed");
        }

        //print diagnostic to console
        fprintf (stderr, "Server: got message: %s\n", buffer);

        //TODO: do useful stuff
    }
}
