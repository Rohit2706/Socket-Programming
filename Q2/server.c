#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <poll.h>

#include "packet.h"

void create_packet(PACKET* sent, int seq){
	sent->size = 0;
	sent->seq = seq;
	sent->ifLast = 0;
	sent->ifData = 0;
	sent->sender = Server;
	sent->reciever = Relay1 + rand()%2;
}

int main(){

	//////////////// SETTING UP SERVER SIDE ///////////////////////////////

	// Create TCP Socket for the server

	int server_socket;

	server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (server_socket< 0)
		error("Error in creating server socket\n");

	printf("Server Socket Created Successfully\n");

	// constructing local address structure for server

	struct sockaddr_in server;
	memset (&server,0,sizeof(server));

	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	// binding the server socket to the server address

	int temp = bind(server_socket, (struct sockaddr*) &server, sizeof(server));
	if (temp < 0)
		error("Error while binding\n");

	printf("Binding Successful\n");

	/////////////////////// SETTING UP RELAY SIDE /////////////////////////////////

	// constructing local address structure for relay node

	int num_relay = 2;

	struct sockaddr_in relay[num_relay];

	memset (&relay[0],0,sizeof(relay[0]));

	relay[0].sin_family = AF_INET;
	relay[0].sin_port = htons(RELAY1_PORT);
	relay[0].sin_addr.s_addr = inet_addr("127.0.0.1");

	memset (&relay[1],0,sizeof(relay[1]));

	relay[1].sin_family = AF_INET;
	relay[1].sin_port = htons(RELAY2_PORT);
	relay[1].sin_addr.s_addr = inet_addr("127.0.0.1");

	////////////////////// SETTING UP REQUIRED DATA/VARIABLES /////////////////////////////

	// Setting up fd channels for select calls

	fd_set active_channels, temp_channels;
 	int fd_max = server_socket;
 	FD_ZERO(&active_channels);
 	FD_ZERO(&temp_channels);

 	FD_SET(server_socket, &active_channels);

 	// define local variables

 	int select_flag;
 	struct timeval timeout;
 	
 	PACKET buffer[BUFFER_SIZE];

 	int expectedSeq = 0;
 	for(int i=0;i<BUFFER_SIZE;i++)
 		buffer[i].seq = -1;

 	// recieved packets
 	PACKET recieved;
 	int recvsize;

 	PACKET ack[20];

 	for(int i=0;i<20;i++)
 		ack[i].seq = -1;

 	struct sockaddr_in rcvsocket;
 	int length = sizeof(rcvsocket);

 	// sent packet

 	PACKET sent;

 	// open the output file

 	FILE* fp;
 	fp = fopen("output.txt", "w");

 	if(fp == NULL)
		error("Error while opening file\n");

	// open the file to write log

	FILE* logfile;
 	logfile = fopen("log.txt", "a");

 	if(logfile == NULL)
		error("Error while opening file\n");

	printf("Printing log data in 'log.txt'\n");

	////////////////////// SELECT, SEND AND READ DATA /////////////////////////


	while(1){

		// read data if server is ready to read

		temp_channels = active_channels;
		timeout.tv_sec = 20;
 		timeout.tv_usec = 0;
		if((select_flag = select(fd_max+1, &temp_channels, NULL, NULL, &timeout))==-1)
 			error("Error while selecting channel\n");
 		if(select_flag == 0)
			break;
 		if(FD_ISSET(server_socket,&temp_channels)){
 			recvsize = recvfrom(server_socket, &recieved, sizeof(recieved), 0, (struct sockaddr*) &rcvsocket, &length);
			if(recvsize == -1)
				error("Error while recieving packet through channel\n");

			if(recvsize>0){
				
				printLog(&recieved, 0, logfile, Server);

				if(expectedSeq == recieved.seq){
					fwrite(recieved.data, sizeof(char), recieved.size, fp);
					expectedSeq++;

					while(buffer[expectedSeq % BUFFER_SIZE].seq == expectedSeq){
						int val = expectedSeq % BUFFER_SIZE;

						fwrite(buffer[val].data, sizeof(char), buffer[val].size, fp);
						expectedSeq++;
						buffer[val].seq = -1;
					}

					create_packet(&ack[recieved.seq%20], recieved.seq);
				}
				else if(buffer[recieved.seq % BUFFER_SIZE].seq == -1){
					buffer[recieved.seq % BUFFER_SIZE] = recieved;
					create_packet(&ack[recieved.seq%20], recieved.seq);
				}

				fflush(fp);
			}
 			
 		}

 		// send data if server is ready to send

		temp_channels = active_channels;

		if((select_flag = select(fd_max+1, NULL, &temp_channels, NULL, NULL))==-1)
 			error("Error while selecting server channel\n");

 		if(FD_ISSET(server_socket,&temp_channels)){
 			for(int i=0;i<20;i++){
 				if(ack[i].seq !=-1){
 					sent = ack[i];
 					int val = sent.reciever - Relay1;
 					printLog(&sent, 1, logfile, Server);
 					if(sendto(server_socket, &sent, sizeof(sent),0, (struct sockaddr*) &relay[val], length)  != sizeof(sent))
						error("Error while sending packet\n");

					ack[i].seq = -1;
 				}
 			}
 		}
	}

	printf("Printing Successful\n");
	printf("File recieved from client successfully\n");
	printf("File recieved has been stored as 'output.txt'\n");

	//////////////////// CLOSE /////////////////////////////

	printf("Now exiting\n");

	close(server_socket);
	fclose(fp);
	fclose(logfile);
	return 0;
}