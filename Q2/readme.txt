
Computer Networks - Q2

Name : Rohit Jain	BITS ID: 2017A7PS0122P

----------------------------------------------------------------------------------------

To run the program run the following commands:

Terminal 1: 	gcc -o server packet.h server.c
		./server

Terminal 2: 	gcc -o relay packet.h relay.c
		./relay

Terminal 3: 	gcc -o client packet.h client.c
		./client


NOTE: Input File at client side should be named "input.txt"
      File recieved at server side is stored as "output.txt"
      Log Data will be printed in "log.txt" with format as specified

------------------------------------------------------------------------------------------

Assumptions/Default Values:

All the macros have been defined in packet.h and can be changed for testing purposes

	MACRO NAME         DEFAULT VALUE         EXPLANATION
	---------------------------------------------------------------------------
	CLIENT_PORT        55557		To bind client socket
	RELAY1_PORT 	   55555		To bind relay1 socket
	RELAY2_PORT 	   55556		To bind relay2 socket
	SERVER_PORT 	   8000			To bind server socket
	PACKET_SIZE        100			Payload data size in a packet
	BUFFER_SIZE        10    		To buffer out of order arrival of packets at server application
	WINDOW_SIZE        4			Sender and Reciever window size as used in SR Protocol
	TIMEOUT_IN_SEC     3  			Timeout interval in seconds to wait before retransmision of data packet by client
	PACKET_DROP_RATE   10			Packet loss simulation at relay nodes

Sequence Number of a packet: Defined as the packet number starting zero. Thus, with PACKET_SIZE 100, a 312B file is divided as:
				Seq:0 	Size:100 bytes
				Seq:1 	Size:100 bytes
				Seq:2 	Size:100 bytes
				Seq:3 	Size:12 bytes

Acknowledgement Number: The server acknowledges the packet by sending the acknowlegment number equal to the sequence number recently recieved packet number.
			Thus, on recieving packet with seq = n, server will send an acknowledgement packet with ack no. = n


NOTE: A timeout of 20 seconds has been assumed at relay and server nodes to detect the closure of socket by client on successful transfer. Thus, when testing
      one should make sure that Drop Rate is not too high that server closes itself after recieving no packet from relay nodes for 20 seconds.
      Same argument is true for Relay nodes as well. 
      You can change this value at Line 136: server.c and Line 122: relay.c when dealing with high drop rates to a large value, say 60 seoconds

      PLEASE WAIT FOR ALL THREE PROGRAMS TO EXIT AND CLOSE THE FILE OR ELSE THERE MIGHT BE SOME DATA AT THE END NOT WRITTEN DUE TO I/O BUFFERING BY OS KERNEL

--------------------------------------------------------------------------------------------

CODE/IMPLEMENTATION EXPLANATION

	At client side
	-----------------------------------------
	Number of sockets: 1
	Communicates with: Relay1, Relay2
	Multiple Timers: Each packet has it's own timer stored in packet structure. Timer is started when the packet is sent and stopped when
			 ack is recieved.
	Window: implemented as a circular array of packets of size WINDOW_SIZE
	window_base: stores the base sequence inside a window
					      --------
		For example: WINDOW: 1 2 3 4 |5 6 7 8 | 9
					      --------
			     window_base = 5
	
	Input File: input.txt
	Create Packets: Whenever SR protocol allows the transmission of new packet, file is read on the fly and new packet is generated and buffered till an
			ack is recieved.
	Send and Recieve Packets: All odd sequenced packets are sent via Relay1
		     		  All even sequenced packets are sent via Relay2
		      		  (using function sendto)

				  Acknowlegement packets are recieved and are not sent again in case of timeout in future (Selctive repeat)
	Retransmission: In case timeout occurs, packet is retransmitted


	At Relay side
	-----------------------------------------
	Number of sockets: 2 (Relay1 and Relay2)
	Communicates with: Client, Server
	Relay1 : Recieves only odd sequenced packets from client
		 Can recieve both odd or even acknowledgement packets from server
	Relay2 : Recieves only even sequenced packets from client
		 Can recieve both odd or even acknowledgement packets from server
	Packet loss: Packet is recieved from client and printed as a log. A random number between 0,100 is generated using rand()%100. If this number is greater than
	             or equal to the PACKET_DROP_RATE, the packet is relayed to server otheriwise the packet is dropped.
	Delay: The relay.c program is set to sleep() before sending packet to server for t amount of milliseconds, 0<=t<=2 where t is generated randomly
	
	Acknowledgement packets do not suffer packet loss and delay


	At server side
	----------------------------------------
	Number of sockets: 1
	Communicates with: Relay1, Relay2
	expectedSeq: Stores the currenty expected packet sequence (=0 at beginning of the program)
	Buffer: implemented as a circular array of packets of size BUFFER_SIZE to store out of order arrival of packets.
	Output File: output.txt
	Create Packets: Creates acknowledgement packets for recieved data packets
	Send Acknowledgements: Server picks Relay1 or Relay2 at random to send the acknowledgement. Thus, Relay1 can recieve even sequenced acknowledgement
			       packet from server.

	File Write: If the currently recieved packet matches the expectedSeq, update expectedSeq and write this packet and
		    while expectedSeq packet exists in buffer, write the packet from buffer and update expectedSeq. Otherwise, if it doesn't match expectedSeq
		    buffer the recieved packet for later use.

		    This way out of order arrival is handled.
			