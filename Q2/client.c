#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <poll.h>

#include "packet.h"


int check_if_end(PACKET* packet){
	for(int i=0;i<WINDOW_SIZE;i++)
		if(packet[i].size != 0)
			return 1;
	return 0;
}

void create_packet(PACKET* packet, int seq, FILE* fp){
	
	fseek(fp, seq*PACKET_SIZE, SEEK_SET);
	packet->size = fread(packet->data, sizeof(char), PACKET_SIZE, fp);
	if(packet->size<0)
		error("Error while reading file\n");

	packet->seq = seq;
	packet->sender = Client;
	packet->reciever = Relay1 +(seq+1)%2;
	packet->ifLast = feof(fp)? 1:0;
	packet->ifData = 1;
}

int main(){

	//////////////// SETTING UP CLIENT SIDE ///////////////////////////////

	// Create TCP Socket for the client

	int client_socket;

	client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (client_socket< 0)
		error("Error in creating client socket\n");

	printf("Client Socket Created Successfully\n");

	// constructing local address structure for client node

	struct sockaddr_in client;

	memset (&client,0,sizeof(client));

	client.sin_family = AF_INET;
	client.sin_port = htons(CLIENT_PORT);
	client.sin_addr.s_addr = inet_addr("127.0.0.1");

	// binding the client socket to the  client address

	int temp = bind(client_socket, (struct sockaddr*) &client, sizeof(client));
	if (temp < 0)
		error("Error while binding\n");

	printf("Binding Successful\n");

	/////////////////////// SETTING UP RELAY SIDE /////////////////////////////////

	// constructing local address structure for relay

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
 	int fd_max = client_socket;
 	FD_ZERO(&active_channels);
 	FD_ZERO(&temp_channels);

 	FD_SET(client_socket, &active_channels);

 	// Defining Timeout
 	struct timeval TIMEOUT;

 	// define local variables

 	int select_flag;

 	// recieved packets
 	PACKET recieved;
 	int recvsize;

 	struct sockaddr_in rcvsocket;
 	int length = sizeof(rcvsocket);

 	// sent packets

 	PACKET packet[WINDOW_SIZE];

 	for(int i=0;i<WINDOW_SIZE;i++)
 		packet[i].size = -1;

 	int window_base = 0;

 	int seq = 0;

 	// open the input file

 	FILE* fp;
 	fp = fopen("input.txt", "r");

 	if(fp == NULL)
		error("Error while opening file\n");

	// open the file to write log

	FILE* logfile;
 	logfile = fopen("log.txt", "w");

 	if(logfile == NULL)
		error("Error while opening file\n");

	printf("Printing log data in 'log.txt'\n");
	printf("Sending file 'input.txt'\n");
	printf("Progress: ");

	fprintf(logfile, "%-10s\t%-10s\t%-20s\t%-11s\t%-10s\t%-10s\t%-10s\n",
			"Node Name", "Event Type", "Timestamp", "Packet Type", "Seq. No", "Source", "Dest");
	fprintf(logfile, "----------------------------------------------------------------------------------------------------------------\n\n");
	////////////////////// SELECT, SEND AND READ DATA /////////////////////////


	while(check_if_end(packet)){
		printf("#");
		fflush(stdout);
		// send data if client is ready to send

		temp_channels = active_channels;

		if((select_flag = select(fd_max+1, NULL, &temp_channels, NULL, NULL))==-1)
 			error("Error while selecting relay channel\n");
 	
 		if(FD_ISSET(client_socket,&temp_channels)){

 			while(packet[seq%WINDOW_SIZE].size ==-1){
 				int i = seq%WINDOW_SIZE;
 				if(seq != window_base && seq >= window_base + 2* WINDOW_SIZE)
 					break;
 					
 				create_packet(&packet[i], seq, fp);
 				seq += 1;

 				if(packet[i].size>0){
 					int val = (packet[i].seq+1)%2;

 					printLog(&packet[i], 1,logfile, Client);

 					if(sendto(client_socket, &packet[i], sizeof(packet[i]),0, (struct sockaddr*) &relay[val], length)  != sizeof(packet[i]))
							error("Error while sending packet\n");
 				}
 			}
 		}

 		// read data if client is ready to read

		temp_channels = active_channels;
		TIMEOUT.tv_sec = TIMEOUT_IN_SEC;
 		TIMEOUT.tv_usec = 0;
		if((select_flag = select(fd_max+1, &temp_channels, NULL, NULL, &TIMEOUT))==-1)
 			error("Error while selecting channel\n");
 		if(select_flag == 0){
 			
 			if(packet[window_base%WINDOW_SIZE].size>0)
 				printLog(&packet[window_base%WINDOW_SIZE], 3, logfile, Client);

 			for(int i=0;i<WINDOW_SIZE;i++){
 				if(packet[i].size>0){
 					int val = (packet[i].seq+1)%2;

 					printLog(&packet[i], 2,logfile, Client);
 					if(sendto(client_socket, &packet[i], sizeof(packet[i]),0, (struct sockaddr*) &relay[val], length)  != sizeof(packet[i]))
							error("Error while sending packet\n");
 				}
 			}
 		}

 		else if(FD_ISSET(client_socket,&temp_channels)){
 			recvsize = recvfrom(client_socket, &recieved, sizeof(recieved), 0, (struct sockaddr*) &rcvsocket, &length);
			if(recvsize == -1)
				error("Error while recieving packet through channel\n");

			if(recvsize>0){
			
				printLog(&recieved, 0,logfile, Client);

				for(int i=0;i<WINDOW_SIZE;i++){
					if(recieved.seq == packet[i].seq){
						packet[i].size = -1;
						break;
					}
				}

				while(packet[window_base%WINDOW_SIZE].size == -1 && window_base<seq){
					window_base += 1;
				}
			}
 		}

	 		// printf("PACKET: %d\n",window_base);
	 		// for(int i=0;i<WINDOW_SIZE;i++)
	 		// 	printf("%d: seq- %d size- %d\n", i, packet[i].seq, packet[i].size);
	 		// printf("\n");
	}

	printf("\nPrinting Successful\n");
	printf("File transfered to server successfully\n");

	//////////////////// CLOSE /////////////////////////////

	close(client_socket);
	fclose(fp);
	fclose(logfile);

	printf("Now exiting\n");
	return 0;
}