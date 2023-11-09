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
#define __USE_BSD	/* use bsd'ish ip header */
#include <sys/socket.h>	/* these headers are for a Linux system, but */
#include <netinet/in.h>	/* the names on other systems are easy to guess.. */
#include <netinet/ip.h>
#define __FAVOR_BSD	/* use bsd'ish tcp header */
#include <netinet/tcp.h>
#include <unistd.h>


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


unsigned short		/* this function generates header checksums */
csum (unsigned short *buf, int nwords)
{
  unsigned long sum;
  for (sum = 0; nwords > 0; nwords--)
    sum += *buf++;
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return ~sum;
}

void send_syn_packet(){
	int s = socket (PF_INET, SOCK_RAW, IPPROTO_TCP);	/* open raw socket */
	  char datagram[4096];	/* this buffer will contain ip header, tcp header,
				   and payload. we'll point an ip header structure
				   at its beginning, and a tcp header structure after
				   that to write the header values into it */
	  struct ip *iph = (struct ip *) datagram;
	  struct tcphdr *tcph = (struct tcphdr *) datagram + sizeof (struct ip);
	  struct sockaddr_in sin;
				/* the sockaddr_in containing the dest. address is used
				   in sendto() to determine the datagrams path */
	
	  sin.sin_family = AF_INET;
	  sin.sin_port = htons (9999);/* you byte-order >1byte header values to network
				      byte order (not needed on big endian machines) */
	  sin.sin_addr.s_addr = inet_addr ("192.168.128.1");
	
	  memset (datagram, 0, 4096);	/* zero out the buffer */
	
	/* we'll now fill in the ip/tcp header values, see above for explanations */
	  iph->ip_hl = 5;
	  iph->ip_v = 4;
	  iph->ip_tos = 0;
	  iph->ip_len = sizeof (struct ip) + sizeof (struct tcphdr);	/* no payload */
	  iph->ip_id = htonl (54321);	/* the value doesn't matter here */
	  iph->ip_off = 0;
	  iph->ip_ttl = 255;
	  iph->ip_p = 6;
	  iph->ip_sum = 0;		/* set it to 0 before computing the actual checksum later */
	  iph->ip_src.s_addr = inet_addr ("1.2.3.4");/* SYN's can be blindly spoofed */
	  iph->ip_dst.s_addr = sin.sin_addr.s_addr;
	  tcph->th_sport = htons (1234);	/* arbitrary port */
	  tcph->th_dport = htons (9999);
	  tcph->th_seq = random ();/* in a SYN packet, the sequence is a random */
	  tcph->th_ack = 0;/* number, and the ack sequence is 0 in the 1st packet */
	  tcph->th_x2 = 0;
	  tcph->th_off = 0;		/* first and only tcp segment */
	  tcph->th_flags = TH_SYN;	/* initial connection request */
	  tcph->th_win = htonl (65535);	/* maximum allowed window size */
	  tcph->th_sum = 0;/* if you set a checksum to zero, your kernel's IP stack
			      should fill in the correct checksum during transmission */
	  tcph->th_urp = 0;
	
	  iph->ip_sum = csum ((unsigned short *) datagram, iph->ip_len >> 1);
	
	/* finally, it is very advisable to do a IP_HDRINCL call, to make sure
	   that the kernel knows the header is included in the data, and doesn't
	   insert its own header into the packet before our data */
	
	  {				/* lets do it the ugly way.. */
	    int one = 1;
	    const int *val = &one;
	    if (setsockopt (s, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
	      printf ("Warning: Cannot set HDRINCL!\n");
	  }
	
	  while (1)
	    {
	      if (sendto (s,		/* our socket */
			  datagram,	/* the buffer containing headers and data */
			  iph->ip_len,	/* total length of our datagram */
			  0,		/* routing flags, normally always 0 */
			  (struct sockaddr *) &sin,	/* socket addr, just like in */
			  sizeof (sin)) < 0)		/* a normal send() */
		printf ("error\n");
	      else
		printf (".");
	    }
	
	  
}


int main(int argc, char* argv[]){
	send_syn_packet();
}
