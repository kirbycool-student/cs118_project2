#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <string.h>
#include "functions.h"

int main(int argc, char *argv[]) {
    int sock, port, nbytes, size;
    struct sockaddr_in serv_addr, client_addr;

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

    //bind the socket
    size = sizeof(serv_addr);
    if ( bind(sock, (struct sockaddr*) &serv_addr, (socklen_t) size) < 0) {
        error("bind error");
    }

    while(1) {
        
        struct packet initPacket;
        //wait for a packet
        fprintf (stderr, "waiting for message \n");

        size = sizeof(client_addr);
        if( nbytes = recvfrom(sock, &initPacket, DATAGRAM_SIZE, 0, 
                                (struct sockaddr *) &client_addr,
                                 &size) < 0)
        {
            error("recvfrom failed");
        }

        //print diagnostic to console
        char addr[256];
        inet_ntop(AF_INET, &(client_addr.sin_addr), addr, INET_ADDRSTRLEN);

        fprintf (stderr, "Sender: got message: %s From: %s : %d\n", initPacket.data, addr, client_addr.sin_port);
        
        initPacket.ack = 1;  
     
        //send the message
        size = sizeof(client_addr);
        if( nbytes = sendto (sock, &initPacket, DATAGRAM_SIZE, 0,
                    (struct sockaddr *) &client_addr , size) < 0)
        {
            error("sendto failed");
        }

        //print diagnostic to console
        inet_ntop(AF_INET, &(client_addr.sin_addr), addr, INET_ADDRSTRLEN); 

        printf ("Sender: Sent ack for : %d To: %s : %d \n", initPacket.seq, addr, client_addr.sin_port);
       // printf ("Sender: Sent message: %s To: %s : %d \n", buffer, addr, client_addr.sin_port);

    }
}
