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

    srand( time(0) ); //seed the rng for probablilty stuff

    //usage
    if (argc < 6) {
       fprintf(stderr,"usage %s hostname port message Pl Pc \n", argv[0]);
       exit(0);
    }

    //SOCKET INITIALIZATION
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
    
    //FILE IO 
    // open file
    FILE * fd = fopen(argv[3],"w");
    if (fd == NULL) // open failed 
    {
        error("failed open file"); 
    }

    int pCorrupt = atoi(argv[4]);
    int pLoss= atoi(argv[5]);

    //init server address stuff
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);
    printf("port: %d \n", ntohs(serv_addr.sin_port));
    
//******************handshake**********************************//
    struct packet handshake;
    initPacket(&handshake);
    strncpy(handshake.data,argv[3],DATA_SIZE);
    handshake.size = strlen(argv[3]);

    //send the message
    int size = sizeof(serv_addr);
    if( nbytes = sendto (sock, &handshake, DATAGRAM_SIZE, 0,
                    (struct sockaddr *) &serv_addr , size) < 0)
    {
        error("sendto failed");
    }

    //print diagnostic to console
    char addr[256];
    inet_ntop(AF_INET, &(serv_addr.sin_addr), addr, INET_ADDRSTRLEN); 
    printf ("Receiver: Sent message: %s To: %s : %d \n", handshake.data, addr, serv_addr.sin_port);
    dump(&handshake);

//******************main loop for receiving data*******************//

    fprintf (stderr, "waiting for message \n");
    struct packet incoming;
    struct packet outgoing;
    initPacket(&incoming);
    initPacket(&outgoing);
    outgoing.ack = 1;
    int cumAck = 0;    

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
        {
            //first pkt is ack of request
            fprintf (stderr, "Receiver: got ack From: %s : %d\n", addr, serv_addr.sin_port);
            dump(&incoming);
        }
        else if (incoming.fin == 1)
        {
            //terminate connection
            fprintf (stderr, "Receiver: got fin :  From: %s : %d\n", addr, serv_addr.sin_port);
            dump(&incoming);
            fclose(fd);
            break;
        } 
        else  
        {
            // data packet
            // fprintf (stderr, "Receiver: got test message From: %s : %d\n", addr, serv_addr.sin_port);
            dump(&incoming);

            if (prob(pCorrupt) || prob(pLoss)) {
                //corrupt packet
                printf("packet was corrupted or lost\n");
                continue;
            }
            else if (incoming.seq == cumAck + 1) {
                //write data to file 
                fwrite(incoming.data,1,incoming.size,fd);  
                cumAck++;
                outgoing.seq = incoming.seq;
            }
            else {
                //out of order packet
                printf("packet is out of order\n");
                outgoing.seq = cumAck;
            }

            //send corresponding ack 
            int size = sizeof(serv_addr);
            if( nbytes = sendto (sock, &outgoing, DATAGRAM_SIZE, 0,
                                (struct sockaddr *) &serv_addr , size) < 0)
            {
                error("sendto failed");
            }    
            
            //print diagnostic to console
            char addr[256];
            inet_ntop(AF_INET, &(serv_addr.sin_addr), addr, INET_ADDRSTRLEN); 
            // printf ("Receiver: Sent ack to: %s : %d \n", addr, serv_addr.sin_port);
            dump(&outgoing);
        }
    }   
}
