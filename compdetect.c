#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include "cJSON.h"
#include <fcntl.h>
#include <time.h>

cJSON*json_dict;

//define headers

//IP hear
struct ipheader
{
    unsigned char ip_hl : 4, ip_v : 4; /* this means that each member is 4 bits */
    unsigned char ip_tos;
    unsigned short int ip_len;
    unsigned short int ip_id;
    unsigned short int ip_off;
    unsigned char ip_ttl;
    unsigned char ip_p;
    unsigned short int ip_sum;
    unsigned int ip_src;
    unsigned int ip_dst;
};
//UDP header
struct udpheader
{
    unsigned short int uh_sport;
    unsigned short int uh_dport;
    unsigned short int uh_len;
    unsigned short int uh_check;
};
//TCP header
struct tcpheader
{
    unsigned short int th_sport;
    unsigned short int th_dport;
    unsigned int th_seq;
    unsigned int th_ack;
    unsigned char th_x2 : 4, th_off : 4;
    unsigned char th_flags;
    unsigned short int th_win;
    unsigned short int th_sum;
    unsigned short int th_urp;
}; 


void asign_from_json(char* jsonFile) {

	FILE *file = fopen(jsonFile, "r");
    if (!file) {
        perror("Failed to open the JSON file");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *json_data = (char *)malloc(file_size + 1);
    if (!json_data) {
        fclose(file);
        perror("Memory allocation failed");
        exit(1);
    }

    fread(json_data, 1, file_size, file);
    fclose(file);
    json_data[file_size] = '\0';

    // Parse the JSON data
    json_dict = cJSON_Parse(json_data);
    free(json_data);

    if (json_dict == NULL) {
        perror("Failed to parse JSON data");
        exit(1);
    }
}

/* this function generates header checksums */
unsigned short 
csum(unsigned short *buf, int nwords)
{
    unsigned long sum;    for (sum = 0; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return ~sum;
}

/* This function sets up the ip and tcp header as it set their pointers */
void setupIPandTCPHeader(struct ipheader *iph, struct tcpheader *tcph, struct sockaddr_in addrSynHead, char *datagram, int destPort)
{
	printf("107");
    iph->ip_hl = 5; // 0101
    iph->ip_v = 4;  // 0100      combined to make one unsignedchar    01010100
    iph->ip_tos = 0;
    iph->ip_len = sizeof(struct ipheader) + sizeof(struct tcphdr); /* no payload */
    printf("ip len %d",iph->ip_len );
    iph->ip_id = (unsigned short)htonl(54321);                               /* the value doesn't matter here */
    iph->ip_off = 0;
    iph->ip_ttl = 255;
    iph->ip_p = 6;
    iph->ip_sum = 0; /* set it to 0 before computing the actual checksum later */
    iph->ip_src = inet_addr("127.0.0.1"); /* SYN's can be blindly spoofed */
    iph->ip_dst = addrSynHead.sin_addr.s_addr;
    tcph->th_sport = htons(1234); /* arbitrary port */
    tcph->th_dport = htons(destPort);
    tcph->th_seq = random(); /* in a SYN packet, the sequence is a ra0ndom */
    tcph->th_ack = 0;        /* number, and the ack sequence is 0 in the 1st packet */
    tcph->th_x2 = 0;
    tcph->th_off = 0;            /* first and only tcp segment */
    tcph->th_flags = TH_SYN;     /* initial connection request */
    tcph->th_win = (unsigned short)htonl(65535); /* maximum allowed window size */
    tcph->th_sum = 0;            /* if you set a checksum to zero, your kernel's IP stack
                            should fill in the correct checksum during transmission */
    tcph->th_urp = 0;

    iph->ip_sum = csum((unsigned short *)datagram, iph->ip_len >> 1);
}

void sendSYNPacket(int sockId,struct ipheader *iph, struct tcpheader *tcph, struct sockaddr_in addrSynHead, char *datagram, int destPort){
  // fill in the tcp and ip header information
    setupIPandTCPHeader(iph, tcph, addrSynHead, datagram, destPort);
    // at this point the way the pointers are setup the the ip and tcp header information SHOULD BE IN THE DATAGRAM
    // send low entropy packet train
     // send the tcp message
    if (sendto(sockId, datagram, iph->ip_len, 0, (struct sockaddr *)&addrSynHead, sizeof(addrSynHead)) < 0)
    {
        printf("error: could not send TCP Syn Head on raw socket\n");
    }
    else
    {
        printf("sent TCP Syn to port: %d\n", destPort);
    }
}

void send_udp_packets(int payload_type) {
    int s;
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // IPv4 or v6
    hints.ai_socktype = SOCK_DGRAM; // Use UDP
    hints.ai_flags = AI_PASSIVE; // Assign local host IP address to socket
    int sleep_time = cJSON_GetObjectItem(json_dict, "InterMeasurementTimeInSeconds")->valueint;
    char* ip = cJSON_GetObjectItem(json_dict, "ServerIPAddress")->valuestring;
   	int portInt = cJSON_GetObjectItem(json_dict, "DestinationPortNumberUDP")->valueint;
   	char port[5];
   	sprintf(port, "%d", portInt);

    if ((status = getaddrinfo(ip, port, &hints, &servinfo) != 0)) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // Set up socket
    s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	int val = 1;
	if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void*)&val, sizeof(val)) < 0)
	{
		perror("setsockopt error\n");
		exit(EXIT_FAILURE);
	}   
	// Specify the source port 
    struct sockaddr_in source_addr;
    source_addr.sin_family = AF_INET;
    source_addr.sin_port = htons(cJSON_GetObjectItem(json_dict, "SourcePortNumberUDP")->valueint); // Set the desired source port
    source_addr.sin_addr.s_addr = INADDR_ANY; // Use any available local IP address

    if (bind(s, (struct sockaddr *)&source_addr, sizeof(source_addr)) == -1) {
        perror("bind");
        close(s);
        exit(1);
    }

    if (connect(s, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("connect");
        exit(1);
    }	
	//set TTL
	int ttl_value = cJSON_GetObjectItem(json_dict, "TTLForUDPPackets")->valueint; // TTL value (change as needed)
	//printf("ttl: %d", ttl_value);
	if (setsockopt(s, IPPROTO_IP, IP_TTL, &ttl_value, sizeof(ttl_value)) == -1) {
	    perror("setsockopt (TTL)");
	    close(s);
	    exit(1);
	}
	//set dont fragment
	int df_flag = IP_PMTUDISC_DO; // Enable the DF flag
	if (setsockopt(s, IPPROTO_IP, IP_MTU_DISCOVER, &df_flag, sizeof(df_flag)) == -1) {
	    perror("setsockopt (DF)");
	    exit(1);
	}
	int num_of_packets =  cJSON_GetObjectItem(json_dict, "NumberOfUDPPacketsInPacketTrain")->valueint;
	for(int i = 0; i < num_of_packets; i++){
	    // Create the packet
	    char packet[cJSON_GetObjectItem(json_dict, "SizeOfUDPPayloadInBytes")->valueint + 2];

	    // Fill the string with null characters
	    memset(packet, '0', sizeof(packet));
	    //create packet id
		unsigned char idByteRight = i & 0xFF;
        unsigned char idByteLeft = (i >> 8); //& 0xFF;
		packet[0] = idByteLeft;
		packet[1] = idByteRight;
        
		
	    // Open /dev/urandom as a source of randomness
	    if(payload_type == 1){
		    int urandom_fd = open("/dev/urandom", O_RDONLY);
		    if (urandom_fd == -1) {
				perror("open /dev/urandom");
				exit(1);
		    }

		    // Read random data from /dev/urandom
		    if (read(urandom_fd, packet, sizeof(packet)) == -1) {
				perror("read /dev/urandom");
				close(urandom_fd);
				exit(1);
		    }

		    close(urandom_fd);
	    }

	   // Send the data to the server
	   // printf("sending low entropy...");
		//printf("Send info: Ip: %s\n Port: %s\n",ip, port);
	    if (sendto(s, packet, sizeof(packet), 0, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
			perror("sendto");
			exit(1);
	    }

    }

    freeaddrinfo(servinfo);
    close(s);
}


int main(int argc, char* argv[]){
	asign_from_json(argv[1]);
	//--setup socket info and options
    struct sockaddr_in addrSynHead;
    // setup SYN head address info
    addrSynHead.sin_family = PF_INET;
    addrSynHead.sin_port = htons(cJSON_GetObjectItem(json_dict, "DestinationPortNumberTCPHeadSYN")->valueint);//replace with json
    addrSynHead.sin_addr.s_addr = inet_addr("192.168.128.1");

    struct sockaddr_in addrSynTail;
    // setup SYN head address info
    addrSynTail.sin_family = PF_INET;
    addrSynTail.sin_port = htons(cJSON_GetObjectItem(json_dict, "DestinationPortNumberTCPTailSYN")->valueint);
    addrSynTail.sin_addr.s_addr = inet_addr("192.168.128.1");

    //open sockets

    //head
    int rawSockSYNHead = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (rawSockSYNHead < 0)
    {
        printf("Unable to create a socket\n");
        exit(EXIT_FAILURE);
    }

    int one = 1;
    if (setsockopt(rawSockSYNHead, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0)
    {
        printf("Warning: Cannot set HDRINCL in head!\n");
        exit(1); // leave the program
    }
 
    //tail
    int rawSockSYNTail = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (rawSockSYNTail < 0)
    {
        printf("Unable to create tail socket\n");
        exit(0);
    }

    one = 1;
    if (setsockopt(rawSockSYNTail, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0)
    {
        printf("Warning: Cannot set HDRINCL in tail!\n");
        exit(1); // leave the program
    }
 
    char datagram[4096];

    struct ipheader *iph = (struct ipheader *)datagram;
    // After the first 20 bytes, comes the tcpheader so point the tcpheader structure to that part
    // of the datagram
    struct tcpheader *tcph = (struct tcpheader *)datagram + sizeof(struct ipheader);
    // make variable to hold SYN head address/port information   
    
    memset(datagram, 0, 4096);
	//TODO setup udp payload send syn adn udps


	//-------SEND THE ENTROPY PACKET TRAINS
	sendSYNPacket(rawSockSYNHead,iph,tcph,addrSynHead,datagram,cJSON_GetObjectItem(json_dict, "DestinationPortNumberTCPHeadSYN")->valueint);
	clock_t start_time_low, end_time_low,start_time_high, end_time_high;
	double elapsed_time_low, elapsed_time_high;

	//start_time_low = clock();
	/*
	send_udp_packets(0);

	memset(datagram, 0, 4096);
	sendSYNPacket(rawSockSYNHead,iph,tcph,addrSynTail,datagram,cJSON_GetObjectItem(json_dict, "DestinationPortNumberTCPHeadSYN")->valueint);
	end_time_low = clock();
	elapsed_time_low = (double)(end_time_low - start_time_low) / CLOCKS_PER_SEC;
    */
}
