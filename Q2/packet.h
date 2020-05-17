#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <poll.h>

#include<sys/time.h>
#include<time.h>

#define CLIENT_PORT 55557
#define RELAY1_PORT 55555
#define RELAY2_PORT 55556
#define SERVER_PORT 8000

#define PACKET_SIZE 100
#define BUFFER_SIZE 10    // to buffer out of order arrival of packets at server application

#define WINDOW_SIZE 4
#define TIMEOUT_IN_SEC 3  // Timeout interval in seconds to wait for before retransmision of data packet by client

#define PACKET_DROP_RATE 10

///////////////////// structure for a packet /////////////////////////////////////

typedef enum{Client, Relay1, Relay2, Server} NODE;

typedef struct PACKET{
	int size;
	int seq;
	int ifLast;
	int ifData;
	NODE sender;
	NODE reciever;
	char data[PACKET_SIZE];
	struct timeval timer;
}PACKET;

//////////////////// Some Common Utility Functions //////////////////////////////////


// utility function to print sender and reciever
char* printNode(NODE node){
	switch(node){
		case Client: return "Client";
		case Relay1: return "Relay1";
		case Relay2: return "Relay2";
		case Server: return "Server";
	}

	return "";
}

// utility function to get time stamp

char* timestamp(char* timestr){
	time_t now = time(NULL);
	struct tm* now_tm = localtime(&now);

	struct timeval timevalue;
	gettimeofday(&timevalue, NULL);

	strftime(timestr, 15, "%2H:%2M:%2S.", now_tm);

	char decimal[7];
	sprintf(decimal, "%-6ld",timevalue.tv_usec);
	strcat(timestr,decimal);
	return timestr;
}

// utility function to print log data

void printLog(PACKET* packet, int event, FILE* logfile, NODE curr){
	char* time = malloc(sizeof(char)*25);
	fseek(logfile, 0 ,SEEK_END);
	fprintf(logfile, "%-10s\t%-10s\t%-20s\t%-11s\t%-10d\t%-10s\t%-10s\n",
			printNode(curr),
			event==0? "R" : event == 1? "S": event==2? "RE": event==3? "TO" : "D",
			timestamp(time), (packet->ifData == 1) ? "Data" : "Ack",
			packet->seq,
			printNode(packet->sender), printNode(packet->reciever));

	fflush(logfile);
	free(time);
}

// Utility function to print an error
void error(char * err){
	perror(err);
	exit(0);
}