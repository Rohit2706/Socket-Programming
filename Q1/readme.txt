
Computer Networks - Q1

Name : Rohit Jain	BITS ID: 2017A7PS0122P

----------------------------------------------------------------------------------------

To run the program run the following commands:

Terminal 1: 	gcc -o server packet.h server.c
		./server

Terminal 2: 	gcc -o client packet.h client.c
		./client


NOTE: Input File at client side should be named "input.txt"
      File recieved at server side is stored as "output.txt"

------------------------------------------------------------------------------------------

Assumptions/Default Values:

All the macros have been defined in packet.h and can be changed for testing purposes

	MACRO NAME         DEFAULT VALUE         EXPLANATION
	---------------------------------------------------------------------------
	SERVER_PORT 	   8000			To bind server socket
	PACKET_SIZE        100			Payload data size in a packet
	BUFFER_SIZE        10    		To buffer out of order arrival of packets at server application
	TIMEOUT            2000  		Timeout interval in milliseconds(2000 ms = 2s) to wait before retransmision of data packet by client
	PACKET_DROP_RATE   10			Packet loss simulation at relay nodes

Sequence Number of a packet: Defined as the byte number of the first byte in the packet starting zero. Thus, with PACKET_SIZE 100, a 312B file is divided as:
				Seq:0 		Size:100 bytes
				Seq:100 	Size:100 bytes
				Seq:200 	Size:100 bytes
				Seq:300 	Size:12 bytes

Acknowledgement Number: The server acknowledges the packet by sending the acknowlegment number equal to the sequence number recently recieved packet number.
			Thus, on recieving packet with seq = n, server will send an acknowledgement packet with ack no. = n


--------------------------------------------------------------------------------------------

CODE/IMPLEMENTATION EXPLANATION

	At client side
	-----------------------------------------
	Number of sockets: 1
	Communicates with: Server
	Timer implementation: Timeout is monitored using poll call for both the channels	
	Input File: input.txt
	Create Packets: Whenever channel is ready for transmission of new packet, file is read on the fly and new packet is generated and buffered till an
			ack is recieved.
	Send and Recieve Packets: Packets are sent as soon as any of the channel is available for send (avaiable in the beginning or when it 
				  recieves ack for current packet.
		      		  (using function send)

	Retransmission: In case timeout occurs, packet is retransmitted


	At server side
	----------------------------------------
	Number of sockets: 1
	Communicates with: Client
	expectedSeq: Stores the currenty expected byte sequence (= 0 at beginning of the program)
	Buffer: implemented as a circular array of packets of size BUFFER_SIZE to store out of order arrival of packets.
	Output File: output.txt

	Packet loss: Packet is recieved from client and printed as log. A random number between [0,100) is generated using rand()%100. If this number is greater than
	             or equal to the PACKET_DROP_RATE, the ack is sent otheriwise the packet is dropped. In case, the buffer is already full, the packet is dropped
		     so as to prevent overflow of buffer.
	
		     Acknowledgement packets do not suffer packet loss.

	Create Packets: Creates acknowledgement packets for recieved data packets
	Send Acknowledgements: Server send the acknowledgement to the channel from which it recieved the packet. 

	File Write: If the currently recieved packet matches the expectedSeq, update expectedSeq and write this packet and
		    while expectedSeq packet exists in buffer, write the packet from buffer and update expectedSeq. Otherwise, if it 
		    doesn't match expectedSeq buffer the recieved packet for later use.

		    This way out of order arrival is handled.
			