#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include  <netdb.h>
#include <fcntl.h>
#include "cJSON.h"
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

cJSON * json_dict;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct ThreadArgs{
	int isHead;
	char* argv;
};

struct pseudo_header
{
u_int32_t source_address;
u_int32_t dest_address;
u_int8_t placeholder;
u_int8_t protocol;
u_int16_t tcp_length;
};

void 
asign_from_json(char *jsonFile)
{

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

	//Parse the JSON data
		json_dict = cJSON_Parse(json_data);
	free(json_data);

	if (json_dict == NULL) {
		perror("Failed to parse JSON data");
		exit(1);
	}
	
}

/* this function generates header checksums */

void 
send_json_over_tcp(char *jsonFile)
{
	int	s;
	int	status;
	struct addrinfo	hints;
	struct addrinfo *servinfo;
	char *ip = cJSON_GetObjectItem(json_dict, "ServerIPAddress")->valuestring;
	int	portInt = cJSON_GetObjectItem(json_dict, "PortNumberTCP_PreProbingPhases")->valueint;
	char port[5];
	sprintf(port, "%d", portInt);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	//ipv4 or v6 AF_INET is v4 AF_INET6 is v6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	//asigns locall host ip address to socket
	if ((status = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
		//replace NULL with an actuall website or IP if you want
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}
	//set up socket
	s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	if (connect(s, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
		perror("Connect error");
		close(s);
	}
	//Read and send the JSON file over TCP
	FILE * json_file = fopen(jsonFile, "r");
	if (json_file == NULL) {
		perror("Error opening JSON file");
		close(s);
		exit(EXIT_FAILURE);
	}
	char buffer[1024];
	size_t bytesRead;
	while ((bytesRead = fread(buffer, 1, sizeof(buffer), json_file)) > 0) {
		if (send(s, buffer, bytesRead, 0) < 0) {
			printf("send error");

		} else {
			printf("json sent");
		}
	}
	fclose(json_file);
	close(s);
}

void 
send_udp_packets(int payload_type)
{
	pthread_mutex_lock(&mutex);
	int	s;
	int	status;
	struct addrinfo	hints;
	struct addrinfo *servinfo;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	//IPv4 or v6
		hints.ai_socktype = SOCK_DGRAM;
	//Use UDP
		hints.ai_flags = AI_PASSIVE;
	//Assign local host IP address to socket
	int	sleep_time = cJSON_GetObjectItem(json_dict, "InterMeasurementTimeInSeconds")->valueint;
	char *ip = cJSON_GetObjectItem(json_dict, "ServerIPAddress")->valuestring;
	int	portInt = cJSON_GetObjectItem(json_dict, "DestinationPortNumberUDP")->valueint;
	char port[5];
	sprintf(port, "%d", portInt);

	if ((status = getaddrinfo(ip, port, &hints, &servinfo) != 0)) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}
	//Set up socket
	s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	//Specify the source port
	struct sockaddr_in source_addr;
	source_addr.sin_family = AF_INET;
	source_addr.sin_port = htons(cJSON_GetObjectItem(json_dict, "SourcePortNumberUDP")->valueint);
	//Set the desired source port
		source_addr.sin_addr.s_addr = INADDR_ANY;
	//Use any available local IP address

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
	int	ttl_value = cJSON_GetObjectItem(json_dict, "TTLForUDPPackets")->valueint;
	//TTL value(change as needed)
		printf("ttl: %d", ttl_value);
	if (setsockopt(s, IPPROTO_IP, IP_TTL, &ttl_value, sizeof(ttl_value)) == -1) {
		perror("setsockopt (TTL)");
		close(s);
		exit(1);
	}
	//set dont fragment
	int	df_flag = IP_PMTUDISC_DO;
	//Enable the DF flag
	if (setsockopt(s, IPPROTO_IP, IP_MTU_DISCOVER, &df_flag, sizeof(df_flag)) == -1) {
		perror("setsockopt (DF)");
		exit(1);
	}
	int		num_of_packets = cJSON_GetObjectItem(json_dict, "NumberOfUDPPacketsInPacketTrain")->valueint;
	for (int i = 0; i < num_of_packets; i++) {
		//Create the packet
		char packet[cJSON_GetObjectItem(json_dict, "SizeOfUDPPayloadInBytes")->valueint];

		//Fill the string with null characters
		memset(packet, '0', sizeof(packet));
		//create packet id
		unsigned char	idByteRight = i & 0xFF;
		unsigned char	idByteLeft = (i >> 8);
		//&0xFF;
		packet[0] = idByteLeft;
		packet[1] = idByteRight;
		//Open / dev / urandom as a source of randomness
		if (payload_type == 1) {
			int	urandom_fd = open("/dev/urandom", O_RDONLY);
			if (urandom_fd == -1) {
				perror("open /dev/urandom");
				exit(1);
			}
			//Read random data from / dev / urandom
			if (read(urandom_fd, packet, sizeof(packet)) == -1) {
				perror("read /dev/urandom");
				close(urandom_fd);
				exit(1);
			}
			close(urandom_fd);
		}
		//Send the data to the server
			// printf("sending low entropy...");
		//printf("Send info: Ip: %s\n Port: %s\n", ip, port);
		if (sendto(s, packet, sizeof(packet), 0, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
			perror("sendto");
			exit(1);
		}
	}

	freeaddrinfo(servinfo);
	close(s);
	pthread_mutex_unlock(&mutex);

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

void 
set_df_flag(char *ip_header)
{
	//Set the DF bit in the IP header
	unsigned short *flags = (unsigned short *)(ip_header + 6);
	*flags |= htons(IP_DF);
}

void 
*send_syn(void* threadArgs)
{
	//setup addrs for SYN head
	//pthread_mutex_lock(&mutex);
	int		portInt = 0;
	struct ThreadArgs *args = (struct ThreadArgs *)threadArgs;
	if (args->isHead == 1) {
		portInt = cJSON_GetObjectItem(json_dict, "DestinationPortNumberTCPHeadSYN")->valueint;
	} else {
		portInt = cJSON_GetObjectItem(json_dict, "DestinationPortNumberTCPTailSYN")->valueint;
	}
	char *ip = cJSON_GetObjectItem(json_dict, "ServerIPAddress")->valuestring;
	int	raw_socket;
	struct sockaddr_in dest_addr;
	char datagram  [4096];
	struct ip *ip_header = (struct ip *)datagram;
	struct tcphdr *tcp_header = (struct tcphdr *)(datagram + sizeof(struct ip));
	struct pseudo_header psh;

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(portInt);	/* you byte-order >1byte
						 * header values to network
						 * byte order (not needed on
						 * big endian machines) */
	dest_addr.sin_addr.s_addr = inet_addr(ip);

	raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
	if (raw_socket == -1) {
		perror("Socke creation failed");
		exit(-1);
	}
	//IP HEADER
	ip_header->ip_v = 4;
	ip_header->ip_hl = 5;
	ip_header->ip_tos = 0;
	ip_header->ip_len = sizeof(struct ip) + sizeof(struct tcphdr);
	ip_header->ip_id = htons(54321);
	ip_header->ip_off = 0;
	ip_header->ip_ttl = 255;
	ip_header->ip_p = IPPROTO_TCP;
	ip_header->ip_sum = 0;
	//Will be filled in later
	ip_header->ip_src.s_addr = inet_addr("192.168.128.2");
	//Replace with source IP address
	ip_header->ip_dst.s_addr = inet_addr(ip);

	//TCP header
	tcp_header->th_sport = htons(12345);
	//Replace with source port
	tcp_header->th_dport = htons(portInt);
	tcp_header->th_seq = rand();
	tcp_header->th_ack = 0;
	tcp_header->th_off = 5;
	tcp_header->th_flags = TH_SYN;
	tcp_header->th_win = htons(65535);
	tcp_header->th_sum = 0;
	//Will be filled in later
	tcp_header->th_urp = 0;
	tcp_header->th_x2 = 0;

	//IP DF
	set_df_flag((char *)ip_header);

	//IP checksum
	ip_header->ip_sum = csum((unsigned short *)datagram, ip_header->ip_len >> 1);

	//TCP checksum
	psh.source_address = inet_addr("192.168.128.2");
	//psh.dest_address = sin.sin_addr.s_addr;
	psh.placeholder = 0;
	psh.protocol = IPPROTO_TCP;
	psh.tcp_length = htons(sizeof(struct tcphdr) + strlen(datagram));

	int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + strlen(datagram);
	//pseudogram = malloc(psize);
	;
	tcp_header->th_sum = csum( (unsigned short*) datagram , psize);
	int	one = 1;
	const int *val = &one;
	if (setsockopt(raw_socket, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) {
		printf("cant set hdrincl");
	}
	if (sendto(raw_socket, datagram, ip_header->ip_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) == -1) {
		perror("Packet sending failed");
		close(raw_socket);
		exit(EXIT_FAILURE);
	}
	close(raw_socket);
	if(args->isHead == 1){
		args->isHead = 0;
		send_udp_packets(0);
		sleep(cJSON_GetObjectItem(json_dict, "InterMeasurementTimeinSeconds")->valueint);
		send_udp_packets(1);
		send_syn(threadArgs);
	}
	//pthread_mutex_unlock(&mutex);
}

void *rst_listener_thread(void *arg)
{
    int rst_socket;
    struct sockaddr_in rst_source_addr;
    struct sockaddr_in rst_dest_addr;
    socklen_t rst_addrlen = sizeof(rst_source_addr);
    char rst_buffer[4096];

    rst_socket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);

    if (rst_socket == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        recvfrom(rst_socket, rst_buffer, sizeof(rst_buffer), 0, (struct sockaddr *)&rst_source_addr, &rst_addrlen);

        // Check for RST flag in TCP header
        struct ip *rst_ip_header = (struct ip *)rst_buffer;
        struct tcphdr *rst_tcp_header = (struct tcphdr *)(rst_buffer + (rst_ip_header->ip_hl << 2));

        if (rst_tcp_header->th_flags & TH_RST)
        {
            printf("Received RST packet\n");
            // Add your RST handling logic here
        }
    }

    close(rst_socket);
    return NULL;
}


int 
main(int argc, char *argv[])
{
	struct ThreadArgs threadArgs;
	threadArgs.isHead = 1;
	threadArgs.argv = argv[1];
	if (argc != 2) {
		printf("Invalid amount of args");
		exit(1);
	}
	asign_from_json(argv[1]);
	send_json_over_tcp(argv[1]);

	//clock_t start_time_head, end_time_head,start_time_tail, end_time_tail;
	//double elapsed_time_head, elapsed_time_tail;

	pthread_t threads[2];
	//int thread_args[5] = {1, 0, 1, 0, 0}; // arguments for the threads
	pthread_create(&threads[0], NULL, send_syn, (void *)&threadArgs);
	pthread_create(&threads[1], NULL, rst_listener_thread, (void *)&threadArgs);

	pthread_join(threads[0],NULL);
	pthread_join(threads[1],NULL);
	/*
	send_json_over_tcp(argv[1]);
	start_time_head = clock();
	//start clock
	send_syn(1);
	send_udp_packets(0);
	sleep(cJSON_GetObjectItem(json_dict, "InterMeasurementTimeinSeconds")->valueint);
	send_udp_packets(1);
	end_time_head = clock();
	start_time_tail = clock();
	send_syn(0);
	end_time_tail = clock();

	elapsed_time_head = (double)(end_time_head - start_time_head) / CLOCKS_PER_SEC;
	elapsed_time_tail = (double)(end_time_tail - start_time_tail) / CLOCKS_PER_SEC;

	if(fabs(elapsed_time_tail - elapsed_time_head) > 0.1){
		printf("No compression dectected");
	}else{
		printf("compression dectected");
	}

	//stop clock`=
	*/
}
