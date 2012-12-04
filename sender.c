#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "functions.h"

int main(int argc, char *argv[]) {
    
    ///////////////*****SETUP****/////////////////
    int sock, port, nbytes, size, base;
    struct sockaddr_in serv_addr, client_addr;

    //initialize the alarm signal
    signal(SIGALRM, catch_alarm);
    
    if (argc < 5) {
         fprintf(stderr,"usage %s port CWind Pl Pc\n", argv[0]);
         exit(1);
    }

    int windowSize = atoi(argv[2]);
    struct packet packets[windowSize];
    int acks[windowSize];
    char fileName[DATA_SIZE];   
    int packetsSent = 0;

    int pCorrupt = atoi(argv[3]);
    int pLoss= atoi(argv[4]);

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    //set socket to not block and set the buffer size
    int flags = fcntl(sock, F_GETFL);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    /*
    int sendbuff;
    int optlen = sizeof(sendbuff);
    if( getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuff, &optlen) != 0 ) {
        error("get sock opt error");
    }
    printf("sndbuf: %d\n", sendbuff);

    sendbuff = DATAGRAM_SIZE * windowSize * 10 / 2;
    if( setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuff, optlen) != 0 ) {
        error("get sock opt error");
    }

    if( getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuff, &optlen) != 0 ) {
        error("get sock opt error");
    }
    printf("sndbuf: %d\n", sendbuff);
    */

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
 



    ///////////////*****HANDSHAKE****/////////////////
    //wait for connections
    struct packet handshake;

    fprintf (stderr, "waiting for message \n");

    size = sizeof(client_addr);
    while(1) {
        if( nbytes = recvfrom(sock, &handshake, DATAGRAM_SIZE, 0, 
                                (struct sockaddr *) &client_addr,
                                 &size) < 0)
        {
            if( errno == EWOULDBLOCK ) {
                continue;
            }
            error("recvfrom failed");
        }
        break;
    }

    //print diagnostic to console
    char addr[256];
    inet_ntop(AF_INET, &(client_addr.sin_addr), addr, INET_ADDRSTRLEN);

    fprintf (stderr, "Sender: got message: %s From: %s : %d\n", handshake.data, addr, client_addr.sin_port);
    dump(&handshake);

    //get the requested filename
    strcpy(fileName, handshake.data);
    
    //init connection by sending an ack   
    //seq stays the same 
    handshake.ack = 1;  
 
    size = sizeof(client_addr);
    while(1) {
        if( nbytes = sendto (sock, &handshake, DATAGRAM_SIZE, 0,
                        (struct sockaddr *) &client_addr , size) < 0)
        {
                if( errno == EWOULDBLOCK ) {
                    continue;
                }
                printf("error: %d\n", errno);
                error("sendto failed");

        }    
        break;
    }


    //print diagnostic to console
    inet_ntop(AF_INET, &(client_addr.sin_addr), addr, INET_ADDRSTRLEN); 
    printf ("Sender: Sent ack for : %d To: %s : %d  ", handshake.seq, addr, client_addr.sin_port);
    dump(&handshake);

    /////// FILE IO /////////
    // open file
    FILE * fd = fopen(fileName,"r");

    if (fd == NULL) // open failed 
    {
        error("failed open file"); 
    }

    ////////////initial window of packets////////////
    base = 1;
    int k;
    for(k = 0; k < windowSize; k++) {

        // get the next chunk of the file
        struct packet p;
        initPacket(&p);

        p.seq = k+1;
        p.size = fread(p.data, 1, DATA_SIZE, fd);
        printf ("Sender: size is %d\n", p.size);
        packets[k] = p;

        //send the packet
        size = sizeof(client_addr);
        while(1) {
            if( nbytes = sendto (sock, &packets[k], DATAGRAM_SIZE, 0,
                (struct sockaddr *) &client_addr , size) < 0)
            {
                if( errno == EWOULDBLOCK ) {
                    continue;
                }
                error("sendto failed");
            }
            break;
        }
        packetsSent++;

        //print diagnostic to console
        inet_ntop(AF_INET, &(client_addr.sin_addr), addr, INET_ADDRSTRLEN); 
        //printf ("Sender: Sent test message to: %s : %d :\n", addr, client_addr.sin_port);
        dump(&packets[k]);

        if(feof(fd) || ferror(fd) ) {
            //done reading file
            break;
        }

   }
   //start the timer
   setTimeout(TIMEOUT);


    ////////****** MAIN LOOP *********///////////
    // wait for acks
    // when we receive an ack, update the packet window and send out the new packets
    while(1) {
        if(timeout) {
            //we timed out, retransmit the window
            printf("packet timed out Seq: \n");
            int k;
            for(k = 0; k < windowSize; k++) {
                //send the packet
                size = sizeof(client_addr);
                while(1) {
                    if( nbytes = sendto (sock, &packets[k], DATAGRAM_SIZE, 0,
                        (struct sockaddr *) &client_addr , size) < 0)
                    {
                        if( errno == EWOULDBLOCK ) {
                            continue;
                        }
                        error("sendto failed");
                    }
                    break;
                }
            }
            setTimeout(TIMEOUT);
        }

        //if file is empty or done reading, terminate connection
        if(feof(fd)) {
        }

        struct packet ack;
        initPacket(&ack);

        //wait for packet
        size = sizeof(client_addr);
        if( nbytes = recvfrom(sock, &ack, DATAGRAM_SIZE, 0, 
                             (struct sockaddr *) &client_addr,
                             &size) < 0)
        {
            if( errno == EWOULDBLOCK ) {
                continue;
            }
            error("recvfrom failed");
        }

        if (prob(pCorrupt) || prob(pLoss)) {
            fprintf(stderr, "packet was corrupted or lost\n");
            continue;
        }

        char addr[256];
        inet_ntop(AF_INET, &(client_addr.sin_addr), addr, INET_ADDRSTRLEN);
        
        /////********* PACKET IS ACK *********////
        if( ack.ack == 1)
        {
            //fprintf (stderr, "Sender: got ack From: %s : %d\n", addr, client_addr.sin_port);
            dump(&ack);
            
            if( feof(fd) && ack.seq == packetsSent) {
                fclose(fd);
                struct packet terminate;
                initPacket(&terminate);
                terminate.fin = 1;
                //send the packet
                size = sizeof(client_addr);
                while(1) {    
                    if( nbytes = sendto (sock, &terminate, DATAGRAM_SIZE, 0,
                            (struct sockaddr *) &client_addr , size) < 0)
                    {
                        if( errno == EWOULDBLOCK ) {
                            continue;
                        }
                        error("sendto failed");
                    }
                    break;
                }
                //print diagnostic to console
                inet_ntop(AF_INET, &(client_addr.sin_addr), addr, INET_ADDRSTRLEN); 
                printf ("Sender: Sent termination: To: %s : %d \n", addr, client_addr.sin_port);
                dump(&terminate);
                break;
            }        

            // update the packets in the window and send new packets
            if(base <= ack.seq) {
                //stop the timeout alarm
                alarm(0);

                for(k=0; k < windowSize; k++) {
                    if(packets[k].seq <= ack.seq ) {
                        if(feof(fd) || ferror(fd) ) {
                            //done reading file
                            printf("done reading file\n");
                            break;
                        }

                        //get the next packet
                        struct packet p;
                        p.seq = base + windowSize;
                        //read the next chunk from the file
                        p.size = fread(p.data, 1, DATA_SIZE, fd);
                        packets[k] = p;

                        //send the packet
                        size = sizeof(client_addr);
                        while(1) {
                            if( nbytes = sendto (sock, &packets[k], DATAGRAM_SIZE, 0, (struct sockaddr *) &client_addr , size) < 0)
                            {
                                if( errno == EWOULDBLOCK ) {
                                    continue;
                                }
                                error("sendto failed");
                            }
                            break;
                        }
                        packetsSent++;

                        //print diagnostic to console
                        inet_ntop(AF_INET, &(client_addr.sin_addr), addr, INET_ADDRSTRLEN); 
                        //printf ("Sender: Sent test message To: %s : %d \n", addr, client_addr.sin_port);
                        dump(&packets[k]);

                        base++;
                    }
                }
                //reset the timer
                setTimeout(TIMEOUT);
            }


        }
        else
        {
            fprintf (stderr, "Sender: message wasn't an ack From: %s : %d\n", addr, client_addr.sin_port);
            dump(&ack);
        }
    
    }

}
