#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <poll.h>

#include "packet.h"

// do initial setup for client channels
void setup(int* channel, int num_channels){

	////////////////////////////////// SETTING UP CLIENT SIDE //////////////////////////////////////

	// Create a TCP Socket for client channels
	
	for(int i=0;i<num_channels;i++){
		channel[i] = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (channel[i] < 0)
			error("Error in creating client channel\n");
	}

	////////////////////////////////// SETTING UP SERVER SIDE //////////////////////////////////////

	// constructing local address structure for server 

	struct sockaddr_in svr_address;
	memset (&svr_address,0,sizeof(svr_address));

	svr_address.sin_family = AF_INET;
	svr_address.sin_port = htons(SERVER_PORT);
	svr_address.sin_addr.s_addr = inet_addr("127.0.0.1");

	// connect client channels to the server socket

	int connect_flag;

	for(int i=0;i<num_channels;i++){
		connect_flag = connect(channel[i], (struct sockaddr*) &svr_address , sizeof(svr_address));
		if (connect_flag < 0)
			error("Error while establishing connection with channel\n");
	}
}

int check_if_end(PACKET* packet, int num_channels){
	for(int i=0;i<num_channels;i++){
		if(packet[i].size!=0)
			return 1;
	}

	return 0;
}
void create_packet(PACKET* packet, int global_seq, int channel, FILE* fp){
	
	packet->size = fread(packet->data, sizeof(char), PACKET_SIZE, fp);
	if(packet->size<0)
		error("Error while reading file\n");

	packet->seq = global_seq;
	packet->ifLast = feof(fp)? 1:0;
	packet->ifData = 1;
	packet->channel = channel;
}

int main(){

	int num_channels = 2;
	int channel[num_channels];

	setup(channel, num_channels);

	// setup the poll fds

	struct pollfd pfds[num_channels];

	for(int i=0;i<num_channels;i++){
		pfds[i].fd = channel[i];
		pfds[i].events = POLLIN;
		pfds[i].revents = 1;
	}

	// send the data to server
	
	FILE* fp = fopen("input.txt", "r");
	if(fp == NULL)
		error("Error while opening file\n");

	int global_seq = 0;

	PACKET packet[num_channels];
	for(int i=0;i<num_channels;i++)
		packet[i].size = 1;
	PACKET recieved;
	int recvsize;
	int rv;
	while(check_if_end(packet, num_channels)){

		// If socket is ready to send

		for(int i=0;i<num_channels;i++){
			if(pfds[i].revents && POLLIN){
				create_packet(&packet[i], global_seq, i, fp);
				global_seq += packet[i].size;

				if(packet[i].size>0){
					if(send(channel[i], &packet[i], sizeof(packet[i]),0) != sizeof(packet[i]))
						error("Error while sending packet\n");

					printf("SENT PKT: Seq. No. %d of size %d bytes from channel %d\n", packet[i].seq, packet[i].size,i);
				}
			}
		}

		rv = poll(pfds,num_channels, TIMEOUT);

		if(rv == -1)
			error("Error while polling\n");
		else if(rv == 0){

			// TIMEOUT occured, resend the packets at channels

			for(int i=0;i<num_channels;i++){
				if(packet[i].size>0){
					if(send(channel[i], &packet[i], sizeof(packet[i]),0) != sizeof(packet[i]))
						error("Error while sending packet\n");

					printf("SENT PKT: Seq. No. %d of size %d bytes from channel %d\n", packet[i].seq, packet[i].size,i);
				}
			}
		}
		else{

			// recieve the packet at appropraite channels

			for(int i=0;i<num_channels;i++){
				if(pfds[i].revents && POLLIN){
					recvsize = recv(pfds[i].fd, &recieved, sizeof(recieved), 0);
					if(recvsize == -1)
						error("Error while recieving packet through channel\n");
					if(recieved.ifData == 1)
						error("Error: ACK packet expected, recieved a DATA packet\n");
					if(recvsize>0)
						printf("RCVD ACK: for PKT with Seq. No. %d from channel %d\n", recieved.seq, i);
				}
			}
		}
	}


	for(int i=0;i<num_channels;i++)
		close(channel[i]);
}