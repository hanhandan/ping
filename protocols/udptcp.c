#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>


#include <time_ms.h>
#include <arguments.h>
#include <protocols.h>

extern args_t arguments;

int udp_create_sock(void){
	int sockd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    printf("create socket from SOCK_DGRAM and IPPROTO_ICMP\n");    
    if(sockd < 0){
		return ERROR_SOCK;
	} else {
		return sockd;
	}
}

int tcp_create_sock(void){
	int sockd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockd < 0){
		return ERROR_SOCK;
	} else {
		return sockd;
	}
}

int udptcp_connect(int sockd){
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(arguments.port)
	};
	inet_aton(arguments.ip_address, &addr.sin_addr);
	
	if(connect(sockd, (struct sockaddr*) &addr, sizeof(addr)) < 0){
		return ERROR_CONNECT;
	} else {
		return NO_ERROR;
	}
}

int udptcp_send(int sockd, void *payload, ping_time_t *start_time, void **data){
    // Initialize the ICMP header
    struct icmphdr icmp_hdr;

    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.type = ICMP_ECHO;
    icmp_hdr.un.echo.id = 1234;
    icmp_hdr.un.echo.sequence = 1;
    
    char packetdata[10240]={0};
    // Initialize the packet data (header and payload)
    memcpy(packetdata, &icmp_hdr, sizeof(icmp_hdr));
    memcpy(packetdata + sizeof(icmp_hdr), payload, arguments.payload_size);
    int size = send(sockd, packetdata, arguments.payload_size+sizeof(icmp_hdr), 0);
	if(size < 0){
		return ERROR_SEND;
	} else {
		*start_time = time_ms();
		return NO_ERROR;
	}
}

int udptcp_recv(int sockd, void *payload, ping_time_t *start_time, float *latency, void *data){
	int recv_len = 0, d;
	ping_time_t end_time;
	proto_result_t result = NO_ERROR;
    
    struct icmphdr icmp_hdr;

    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.type = ICMP_ECHO;
    icmp_hdr.un.echo.id = 1234;
    icmp_hdr.un.echo.sequence = 1;
    

	void *buffer = malloc(arguments.payload_size + sizeof(icmp_hdr));
	void *cursor = buffer;
	do {
		d = recv(sockd, cursor, arguments.payload_size + sizeof(icmp_hdr) - recv_len, MSG_DONTWAIT);
		if(d < 0){
			if(errno != EAGAIN){
				result = ERROR_RECV;
			}
		} else {
			recv_len += d;
		}
	
		end_time = time_ms();
		*latency = compute_latency(&end_time, start_time);
		if(*latency > arguments.timeoutMS){
			result = ERROR_TIMEOUT;
		}
	} while(recv_len < arguments.payload_size && result == NO_ERROR);

    printf("recv result is %d\n",result);
    if(result == NO_ERROR && memcmp(buffer + sizeof(icmp_hdr), payload, arguments.payload_size) != 0){
		result = ERROR_DATACORRUPT;
	}

	free(buffer);

	return result;
}

protocol_stack_t udp_stack = {
	.sock    = &udp_create_sock,
	.connect = &udptcp_connect,
	.send    = &udptcp_send,
	.recv    = &udptcp_recv
};

protocol_stack_t tcp_stack = {
	.sock    = &tcp_create_sock,
	.connect = &udptcp_connect,
	.send    = &udptcp_send,
	.recv    = &udptcp_recv
};

