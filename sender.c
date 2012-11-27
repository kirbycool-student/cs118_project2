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
    int sock, port, nbytes, size, base;
    struct sockaddr_in serv_addr, client_addr;
    struct packet packets[WINDOW_SIZE];
    int acks[WINDOW_SIZE];
    char fileName[DATA_SIZE];



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


    //wait for connections
    struct packet initPacket;
    bzero(initPacket.data,DATA_SIZE);

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

    //get the requested filename
    strcpy(fileName, initPacket.data);
    

    //init connection by sending an ack   
    //seq stays the same 
    initPacket.ack = 1;  
    bzero(initPacket.data,DATA_SIZE);
 
    size = sizeof(client_addr);
    if( nbytes = sendto (sock, &initPacket, DATAGRAM_SIZE, 0,
                    (struct sockaddr *) &client_addr , size) < 0)
    {
        error("sendto failed");
    }

    //print diagnostic to console
    inet_ntop(AF_INET, &(client_addr.sin_addr), addr, INET_ADDRSTRLEN); 
    printf ("Sender: Sent ack for : %d To: %s : %d  ", initPacket.seq, addr, client_addr.sin_port);

    /////// FILE IO /////////
    // open file
    FILE * fd = fopen(fileName,"r");

    if (fd == NULL) // open failed 
    {
        error("failed open file"); 
    }

    //initialize and send the first window of packets
    base = 1;
    int k;
    for(k = 0; k < WINDOW_SIZE; k++) {
        acks[k] = 0;

        // get the next chunk of the file
        char buffer[DATA_SIZE];
        if( !fread(buffer, 1, DATA_SIZE, fd) ) {
            //done reading file
            break;
        }

        struct packet p;
        p.ack = 0;
        p.seq = k+1;
        strcpy(p.data, buffer);
        packets[k] = p;
    }


        /*//create test packets
        int k;
        for(k = 0; k < WINDOW_SIZE; k++) {
            acks[k] = 0;

            struct packet p;
            p.ack = 0;
            p.seq = k+1;
            strcpy(p.data, "this is a test");
            packets[k] = p;
        }
        */

    ///// send the first packets
    for(k = 0; k < WINDOW_SIZE; k++) {
        
        //send the message
        size = sizeof(client_addr);
        if( nbytes = sendto (sock, &packets[k], DATAGRAM_SIZE, 0,
                (struct sockaddr *) &client_addr , size) < 0)
        {
            error("sendto failed");
        }

        //print diagnostic to console
        inet_ntop(AF_INET, &(client_addr.sin_addr), addr, INET_ADDRSTRLEN); 
        printf ("Sender: Sent test message for : %d To: %s : %d With Contents:\n%s\n", packets[k].seq, addr, client_addr.sin_port, packets[k].data);

    }

    ////////****** MAIN LOOP *********///////////
    // wait for acks
    // when we receive an ack, update the packet window and send out the new packets
    while(1) {
        
        struct packet ack;
        int eof = 0;

        //wait for packet
        size = sizeof(client_addr);
        if( nbytes = recvfrom(sock, &ack, DATAGRAM_SIZE, 0, 
                             (struct sockaddr *) &client_addr,
                             &size) < 0)
        {
            error("recvfrom failed");
        }

        //print diagnostic to console
        char addr[256];
        inet_ntop(AF_INET, &(client_addr.sin_addr), addr, INET_ADDRSTRLEN);
        if( ack.ack == 1)
        {
            fprintf (stderr, "Sender: got ack for : %d From: %s : %d\n", ack.seq, addr, client_addr.sin_port);

            if(ack.seq == base+WINDOW_SIZE && eof == 1)
                break;
            
            // update the packets in the window and send new packets
            if(base <= ack.seq) {
                for(k=0; k < WINDOW_SIZE; k++) {
                    if(packets[k].seq <= ack.seq ) {
                        acks[k] = 0;

                        //read the next chunk from the file
                        char buffer[DATA_SIZE];
                        if( !fread(buffer, 1, DATA_SIZE, fd) ) {
                            //done reading file
                            eof = 1;
                            break;
                        }

                        //get the next packet
                        struct packet p;
                        p.ack = 0;
                        p.seq = base + WINDOW_SIZE;
                        strcpy(p.data, buffer);

                        packets[k] = p;

                        //send the packet
                        size = sizeof(client_addr);
                        if( nbytes = sendto (sock, &packets[k], DATAGRAM_SIZE, 0,
                                (struct sockaddr *) &client_addr , size) < 0)
                        {
                            error("sendto failed");
                        }

                        //print diagnostic to console
                        inet_ntop(AF_INET, &(client_addr.sin_addr), addr, INET_ADDRSTRLEN); 
                        printf ("Sender: Sent test message for : %d To: %s : %d \n", packets[k].seq, addr, client_addr.sin_port);

                        base++;
                    }
                }
            }


        }
        else
        {
            fprintf (stderr, "Sender: message wasn't an ack From: %s : %d\n", addr, client_addr.sin_port);
        }
    
    }

}
