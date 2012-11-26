#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <netdb.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <string.h>
#include "functions.h"

int main(int argc, char *argv[]) {

//******************initializations**********************************//
    int sock, port, nbytes;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    //usage
    if (argc < 4) {
       fprintf(stderr,"usage %s hostname port message\n", argv[0]);
       exit(0);
    }
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { 
        error("ERROR opening socket");
    }
    port = atoi(argv[2]);
    server = gethostbyname(argv[1]); 
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);
    printf("port: %d \n", ntohs(serv_addr.sin_port));
    
//******************handshake**********************************//
    struct packet initPacket;
    initPacket.ack = 0;
    initPacket.seq = 1;
    bzero(initPacket.data,DATA_SIZE);
    strncpy(initPacket.data,argv[3],DATA_SIZE);

    //send the message
    int size = sizeof(serv_addr);
    if( nbytes = sendto (sock, &initPacket, DATAGRAM_SIZE, 0,
                    (struct sockaddr *) &serv_addr , size) < 0)
    {
        error("sendto failed");
    }

    //print diagnostic to console
    char addr[256];
    inet_ntop(AF_INET, &(serv_addr.sin_addr), addr, INET_ADDRSTRLEN); 
    printf ("Receiver: Sent message: %s To: %s : %d \n", initPacket.data, addr, serv_addr.sin_port);

//******************main loop for receiving data*******************//

    fprintf (stderr, "waiting for message \n");
    struct packet incoming;
    struct packet outgoing;
    outgoing.ack = 1;
    bzero(incoming.data,DATA_SIZE);
    bzero(outgoing.data,DATA_SIZE);

    while(1) {

        //wait for a packet
        size = sizeof(serv_addr);
        if( nbytes = recvfrom(sock, &incoming, DATAGRAM_SIZE, 0, 
                                (struct sockaddr *) &serv_addr,
                                 &size) < 0)
        {
            error("recvfrom failed");
        }

        //print diagnostic to console
        char addr[256];
        inet_ntop(AF_INET, &(serv_addr.sin_addr), addr, INET_ADDRSTRLEN);

        if (incoming.ack == 1) 
        //first pkt is ack of request
        {
            fprintf (stderr, "Receiver: got ack for: %d From: %s : %d\n", incoming.seq, addr, serv_addr.sin_port);
        }
        else  
        // data packet
        {
            fprintf (stderr, "Receiver: got test message for: %d From: %s : %d\n", incoming.seq, addr, serv_addr.sin_port);

            //send corresponding ack 
            outgoing.seq = incoming.seq;
            int size = sizeof(serv_addr);
            if( nbytes = sendto (sock, &outgoing, DATAGRAM_SIZE, 0,
                                (struct sockaddr *) &serv_addr , size) < 0)
            {
                error("sendto failed");
            }    

            //print diagnostic to console
            char addr[256];
            inet_ntop(AF_INET, &(serv_addr.sin_addr), addr, INET_ADDRSTRLEN); 
            printf ("Receiver: Sent ack for: %d To: %s : %d \n", outgoing.seq, addr, serv_addr.sin_port);
        }
    }   
}
