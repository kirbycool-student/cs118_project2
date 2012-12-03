#include <stdio.h>
#include <signal.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>

#define DATAGRAM_SIZE 1000
#define HEADER_SIZE  (2 * sizeof(short) + 2 * sizeof(int))
#define DATA_SIZE (DATAGRAM_SIZE - HEADER_SIZE)
#define DEBUG 1
#define TIMEOUT 5000 //in microseconds

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
    int size;
    char data[DATA_SIZE]; 
};

void initPacket(struct packet * pkt)
{
    pkt->ack = 0;
    pkt->fin = 0;
    pkt->seq = 0;
    pkt->size = 0;
    memset(pkt->data,0,DATA_SIZE);
}

void dump(struct packet * pkt)
{
    if (DEBUG) {
        printf("ack: %d, fin: %d, seq: %d, size: %d \n",pkt->ack,pkt->fin,pkt->seq,pkt->size);
    }
}

//return 1 with probability a and 0 otherwise. SEED THE RNG FIRST
int prob( int a ) {
  return ((rand () % 100) < a) ? 1 : 0;
}

//signal timeout stuff
volatile sig_atomic_t timeout = 0;


void catch_alarm (int sig) /* signal handler */
{
    timeout = 1;
    signal (sig, catch_alarm);
}

unsigned int setTimeout(unsigned int microseconds) {
    timeout = 0;
    struct itimerval old, current;
    current.it_interval.tv_usec = 0;
    current.it_interval.tv_sec = 0;
    current.it_value.tv_usec = (long int) microseconds; 
    current.it_value.tv_sec = 0;
    if (setitimer (ITIMER_REAL, &current, &old) < 0)
        return 0;
    else
        return old.it_value.tv_sec;
}

