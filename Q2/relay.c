#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <poll.h>

#include "packet.h"

int main(){

	//////////////// SETTING UP RELAY SIDE ///////////////////////////////

	// Create TCP Socket for the relay
	int num_relay = 2;

	int relay_socket[num_relay];

	for(int i=0;i<num_relay;i++){
		relay_socket[i] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (relay_socket[i]< 0)
			error("Error in creating relay socket\n");
	}

	printf("Relay1 and Relay2 Sockets Created Successfully\n");

	// constructing local address structure for relay node

	struct sockaddr_in relay[num_relay];

	memset (&relay[0],0,sizeof(relay[0]));

	relay[0].sin_family = AF_INET;
	relay[0].sin_port = htons(RELAY1_PORT);
	relay[0].sin_addr.s_addr = inet_addr("127.0.0.1");

	memset (&relay[1],0,sizeof(relay[1]));

	relay[1].sin_family = AF_INET;
	relay[1].sin_port = htons(RELAY2_PORT);
	relay[1].sin_addr.s_addr = inet_addr("127.0.0.1");

	// binding the relay socket to the  relay address

	for(int i=0;i<num_relay;i++){
		int temp = bind(relay_socket[i], (struct sockaddr*) &relay[i], sizeof(relay[i]));
		if (temp < 0)
			error("Error while binding\n");
	}

	printf("Binding Successful\n");

	/////////////////////// SETTING UP CLIENT SIDE /////////////////////////////////

	// constructing local address structure for client

	struct sockaddr_in client;
	memset (&client,0,sizeof(client));

	client.sin_family = AF_INET;
	client.sin_port = htons(CLIENT_PORT);
	client.sin_addr.s_addr = inet_addr("127.0.0.1");

	/////////////////////// SETTING UP SERVER SIDE /////////////////////////////////

	// constructing local address structure for server

	struct sockaddr_in server;
	memset (&server,0,sizeof(server));

	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	////////////////////// SETTING UP REQUIRED DATA/VARIABLES /////////////////////////////

	// Setting up fd channels for select calls

	fd_set active_channels, temp_channels;
 	int fd_max = relay_socket[num_relay-1];
 	FD_ZERO(&active_channels);
 	FD_ZERO(&temp_channels);

 	for(int i=0;i<num_relay;i++)
 		FD_SET(relay_socket[i], &active_channels);

 	struct timeval TIMEOUT;

 	// define local variables

 	int select_flag;

 	// sent and recieved packets
 	PACKET recieved[num_relay];

 	for(int i=0;i<num_relay;i++)
 		recieved[i].size = -1;

 	int recvsize[num_relay];

 	struct sockaddr_in rcv_socket;
 	int length = sizeof(rcv_socket);

 	// open the file to write log

	FILE* logfile;
 	logfile = fopen("log.txt", "a");

 	if(logfile == NULL)
		error("Error while opening file\n");

	printf("Waiting to recieve communication\n");
	printf("Printing log data in 'log.txt'\n");

	////////////////////// SELECT, SEND AND READ DATA /////////////////////////

	while(1){

		temp_channels = active_channels;
		TIMEOUT.tv_sec = 20;
 		TIMEOUT.tv_usec = 0;
 		if((select_flag = select(fd_max+1, &temp_channels, NULL, NULL, &TIMEOUT))==-1)
 			error("Error while selecting relay channel\n");

 		if(select_flag == 0)
 			break;

 		for(int i=0;i<num_relay;i++){

	 		if(FD_ISSET(relay_socket[i],&temp_channels)){
				recvsize[i] = recvfrom(relay_socket[i], &recieved[i], sizeof(recieved[i]), 0, (struct sockaddr*) &rcv_socket, &length);
				if(recvsize[i] == -1)
					error("Error while recieving packet through channel\n");
				printLog(&recieved[i], 0, logfile, Relay1+i);
			}
		}

		temp_channels = active_channels;
 		if(select(fd_max+1, NULL, &temp_channels, NULL, NULL)==-1)
 			error("Error while selecting relay channel\n");

 		for(int i=num_relay-1;i>=0;i--){
	 		if(FD_ISSET(relay_socket[i],&temp_channels) && recieved[i].size!=-1){
				
				if(recieved[i].sender == Client){
					if(rand()%100 >= PACKET_DROP_RATE){
						recieved[i].sender = Relay1+i;
						recieved[i].reciever = Server;
						sleep((rand()%2000)/1000000);
						printLog(&recieved[i], 1, logfile, Relay1+i);
						if(sendto(relay_socket[i], &recieved[i], sizeof(recieved[i]),0, (struct sockaddr*) &server, length)  != sizeof(recieved[i]))
								error("Error while sending packet\n");
					}
					else
						printLog(&recieved[i], 4, logfile, Relay1+i);
				}
				else{
					recieved[i].sender = Relay1+i;
					recieved[i].reciever = Client;
					sleep((rand()%2000)/1000000);
					printLog(&recieved[i], 1, logfile, Relay1+i);
					if(sendto(relay_socket[i], &recieved[i], sizeof(recieved[i]),0, (struct sockaddr*) &client, length)  != sizeof(recieved[i]))
						error("Error while sending packet\n");
				
				}
				recieved[i].size = -1;
			}
		}
		
	}

	printf("Printing Successful\n");
	printf("Now exiting\n");
	
	//////////////////// CLOSE /////////////////////////////

	for(int i=0;i<num_relay;i++)
		close(relay_socket[i]);

	fclose(logfile);
	return 0;
}