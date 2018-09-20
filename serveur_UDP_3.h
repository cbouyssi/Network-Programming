#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#define DATASIZE 1454
#define MAXSIZE 1460


typedef struct frag {
   char seq[6];
   char data[MAXSIZE-6];
} *FRAG;


int tripleHandShake (int desc_co, char buffer[], char* port_data, struct sockaddr_in * client, socklen_t alen, struct timeval * RTT);
int rcvACK(int desc_data, struct sockaddr_in adresse_data, socklen_t alen, char * buffer);
int data_exchange(int desc_data, FRAG message, struct sockaddr_in  client, socklen_t alen, char buffer[], struct timeval RTT);
