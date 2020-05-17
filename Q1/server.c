#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <poll.h>
#include <sys/types.h>

#include "packet.h"

// do initial setup for socket
int setup(int* channel, int num_channels){
	
	////////////////////// SETTING UP SERVER SIDE /////////////////////////////////

	// Create a TCP Socket for server

	int svr_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(svr_socket < 0)
		error("Error while creating server socket\n"); 
	
	// constructing address structure for server

	struct sockaddr_in svr_address, clt_address;

	memset (&svr_address, 0, sizeof(svr_address));
	svr_address.sin_family = AF_INET;
	svr_address.sin_port = htons(SERVER_PORT);
	svr_address.sin_addr.s_addr = htonl(INADDR_ANY);

	// binding the server socket to the server address
	int temp = bind(svr_socket, (struct sockaddr*) &svr_address, sizeof(svr_address));
	if (temp < 0)
		error("Error while binding\n");

	temp = listen(svr_socket, num_channels);
	if (temp < 0)
		error("Error while listening\n");

	//////////////////////////// ACCEPTING CLIENT SOCKETS //////////////////////////////

	// Accept client channels
 	int clt_length = sizeof(clt_address);

 	for(int i=0;i<num_channels;i++){
	 	channel[i] = accept(svr_socket, (struct sockaddr*) &clt_address, &clt_length);
	 	
	 	if(clt_length < 0)
	 		error("Error while accepting client\n"); 
 	}
 	return svr_socket;
}

int recieve(int channel, FILE* fp, PACKET* recieved){
	int recvsize;

	recvsize = recv(channel, recieved, sizeof(PACKET), 0);
	if(recvsize == -1)
		error("Error while recieving packet through channel\n");
	if(recieved->ifData == 0)
		error("Error: DATA packet expected, recieved an ACK packet\n");
	return recvsize;
}

void create_packet(PACKET* sent, int seq, int channel){
	sent->size = 0;
	sent->seq = seq;
	sent->ifLast = 0;
	sent->ifData = 0;
	sent->channel = channel;
}

int main (){
	
	int num_channels = 2;
	int channel[num_channels];

	int svr_socket = setup(channel, num_channels);

 	// initialize packets and buffer to handle out of order packets

 	PACKET sent, recieved;
 	int expectedSeq = 0;

 	PACKET buffer[BUFFER_SIZE];
 	
 	for(int i=0;i<BUFFER_SIZE;i++)
 		buffer[i].seq = -1;

 	// select, read and send to clients

 	FILE* fp = fopen("output.txt", "w");

 	fd_set read_channels, temp_channels;
 	int fd_max = channel[num_channels-1];
 	FD_ZERO(&read_channels);
 	FD_ZERO(&temp_channels);

 	for(int i=0;i<num_channels;i++)
 		FD_SET(channel[i], &read_channels);
 	
 	while(1){

 		// if channel is ready to read, read data packeta and send ack or drop the packet

 		temp_channels = read_channels;
 		if(select(fd_max+1, &temp_channels, NULL, NULL, NULL)==-1)
 			error("Error while selecting client channel\n");

 		int i;
 		for(i=0;i<num_channels;i++){
 			if(FD_ISSET(channel[i],&temp_channels)){
 				if(recieve(channel[i], fp, &recieved)==0)
 					break;

 				printf("RCVD PKT: Seq. No. %d of size %d bytes from channel %d\n", recieved.seq, recieved.size,i);

 				if(rand()%100>=PACKET_DROP_RATE){
	 				create_packet(&sent, recieved.seq, i);

	 				// if it is the expected packet or there is still space left in buffer, send ack

					if(expectedSeq == recieved.seq || buffer[(recieved.seq/PACKET_SIZE) % BUFFER_SIZE].seq == -1){
		 				if(send(channel[i], &sent, sizeof(sent),0) != sizeof(sent))
							error("Error while sending packet\n");

						printf("SENT ACK: for PKT with Seq. No. %d from channel %d\n", recieved.seq, i);
					}

					// if it is the expected packet, write this packet and the buffered packets in order
					// else if buffer still has space, buffer the packet
					
					if(expectedSeq == recieved.seq){
						fwrite(recieved.data, sizeof(char), recieved.size, fp);
						expectedSeq += recieved.size;

						while(buffer[(expectedSeq/PACKET_SIZE) % BUFFER_SIZE].seq == expectedSeq){
							int val = (expectedSeq/PACKET_SIZE) % BUFFER_SIZE;

							fwrite(buffer[val].data, sizeof(char), buffer[val].size, fp);
							expectedSeq += buffer[val].size ;
							buffer[val].seq = -1;
						}
					}
					else if(buffer[(recieved.seq/PACKET_SIZE) % BUFFER_SIZE].seq == -1){
						buffer[(recieved.seq/PACKET_SIZE) % BUFFER_SIZE] = recieved;
					}
				} 				
 			}	
 		}
 		if(i!=num_channels)
 			break;
 	}

 	for(int i=0;i<num_channels;i++)
 		close(channel[i]);

 	close(svr_socket);
 	fclose(fp);
}
