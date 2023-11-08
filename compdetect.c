#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <time.h>
#include "cJSON.h"
//later this info comes from config file
cJSON*json_dict;

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



int send_syn_packet() {
	int s = socket (PF_INET, SOCK_RAW, IPPROTO_TCP);	/* open raw socket */
	char buffer[4096];	/* this buffer will contain ip header, tcp header,
		   and payload. we'll point an ip header structure
		   at its beginning, and a tcp header structure after
		   that to write the header values into it */
	struct ip *ip_header = (struct ip *) buffer;
	struct tcphdr *tcp_header = (struct tcphdr *) buffer + sizeof (struct ip);
	struct sockaddr_in sin;
		/* the sockaddr_in containing the dest. address is used
		   in sendto() to determine the datagrams path */

	sin.sin_family = AF_INET;
	sin.sin_port = htons (9999);/* you byte-order >1byte header values to network
		      byte order (not needed on big endian machines) */
	sin.sin_addr.s_addr = inet_addr ("192.168.128.3");

	memset (buffer, 0, 4096);	/* zero out the buffer */

    // Fill in the IP header
    ip_header->ip_hl = 5;//header length
    ip_header->ip_v = 4;//ipv4
    ip_header->ip_tos = 0;//
    ip_header->ip_len = sizeof(struct ip) + sizeof(struct tcphdr);
    ip_header->ip_id = 0;
    ip_header->ip_off = 0;
    ip_header->ip_ttl = 255;//TODO replace with json
    ip_header->ip_p = 6; // can be tcp (6), udp(17), icmp(1)
    ip_header->ip_sum = 0;// compute check sum later
    ip_header->ip_src.s_addr = inet_addr("1.2.3.4");//TODO replace with json
    ip_header->ip_dst.s_addr = sin.sin_addr.s_addr;;// fill main dest_addr with "192.168.128.3" 

    // Fill in the TCP header
    tcp_header->th_sport = htons(1234);
    tcp_header->th_dport = htons(9999);
    tcp_header->th_seq = 0;
    tcp_header->th_ack = 0;
    tcp_header->th_off = 5;
    tcp_header->th_flags = 1;
    tcp_header->th_win = htons(65535); // Maximum window size
    tcp_header->th_sum = 0;
    tcp_header->th_urp = 0;

    ip_header->ip_sum = csum ((unsigned short *) buffer, ip_header->ip_len >> 1);

    int one = 1;//make sure that the kernel knows the header is included in the data
    const int *val = &one;
    if (setsockopt (s, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
      printf ("Warning: Cannot set HDRINCL!\n");

  /*  // Calculate TCP checksum
    unsigned short *ptr = (unsigned short *)tcp_header;
    int tcp_header_size = sizeof(struct tcphdr);
    int tcp_checksum_size = 4096 - sizeof(struct ip);
    unsigned int sum = 0;
    while (tcp_checksum_size > 1) {
        sum += *ptr++;
        tcp_checksum_size -= 2;
    }
    if (tcp_checksum_size > 0) {
        sum += *((unsigned char *)ptr);
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    tcp_header->th_sum= (unsigned short)(~(sum + (sum >> 16)));
*/
    // Send the SYN packet
    while (1)
    {
		if (sendto (s,		/* our socket */
		buffer,	/* the buffer containing headers and data */
		ip_header->ip_len,	/* total length of our datagram */
		0,		/* routing flags, normally always 0 */
		(struct sockaddr *) &sin,	/* socket addr, just like in */
		sizeof (sin)) < 0)		/* a normal send() */
			printf ("error\n");
		else
			printf (".");
	}
    
      return 0;
}

int main(int argc, char* argv[]){
	send_syn_packet();
}
