#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <poll.h>

#define PACKET_SIZE 100
#define TIMEOUT 2000
#define PACKET_DROP_RATE 10
#define BUFFER_SIZE 10

#define SERVER_PORT 8000


///////////////////// structure for a packet //////////////////////////////
typedef struct PACKET{
	int size;
	int seq;
	int ifLast;
	int ifData;
	int channel;
	char data[PACKET_SIZE];
}PACKET;

// Utility function to print an error

void error(char * err){
	perror(err);
	exit(0);
}
