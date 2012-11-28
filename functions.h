#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#define DATAGRAM_SIZE 1000
#define HEADER_SIZE  2 * sizeof(short) + sizeof(int)
#define DATA_SIZE (DATAGRAM_SIZE - HEADER_SIZE)
#define WINDOW_SIZE 4

void error(char *msg) 
{
    perror(msg);
    exit(1);
}

struct packet
{
    short ack;
    short fin;
    int seq;
    char data[DATA_SIZE]; 
};
