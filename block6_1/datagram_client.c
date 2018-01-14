#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include "marshalling.h"

#define NTP_PORT "123"
#define NTP_SECOND_OFFSET 2208988800

struct timeref_t {
	int err;
	unsigned long c_s;
	unsigned long c_r;
	unsigned long s_r;
	unsigned long s_s;
};

unsigned long getTime() {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	unsigned long s = (unsigned long)ts.tv_sec;
	unsigned long ms = (unsigned long)(ts.tv_nsec / 1.0e6);
	unsigned long result = (s << 32) | ms;
	return result;
}

struct timeref_t ntpRequest(char *host, ntp_payload_t *payload) {
	struct addrinfo hints, *res;
	int status;
	struct timeref_t times;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if((status = getaddrinfo(host, NTP_PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", strerror(status));
		times.err = -1;
		return times;
	}

	int sockfd;
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	unsigned char msg[48];
	marshal(&msg[0], payload);
	
	times.c_s = getTime();
	//clock_gettime(CLOCK_REALTIME, &times.c_s);

	if(sendto(sockfd, msg, sizeof msg, 0, res->ai_addr, res->ai_addrlen) <= 0) {
		fprintf(stderr, "recv: %s\n", strerror(errno));
		times.err = -1;
		return times;
	}

	unsigned char response[48];

	if((recvfrom(sockfd, response, 48, MSG_WAITALL, res->ai_addr, &res->ai_addrlen)) <= 0) {
		fprintf(stderr, "recv: %s\n", strerror(errno));
		times.err = -1;
		return times;
	}
	
	times.c_r = getTime();
	//clock_gettime(CLOCK_REALTIME, &times.c_r);

	unmarshal(payload, &response[0]);
	freeaddrinfo(res);
	close(sockfd);

	return times;
}

unsigned long calculateDelay(struct timeref_t *times){ 
	return ((times->s_r - times->c_s) + (times->s_s - times->c_r)) / 2;
}

unsigned long calculateOffset(struct timeref_t *times){ 
	return ((times->c_r - times->c_s) - (times->s_s - times->s_r));
}


void addOffsets(struct timeref_t *times) {
	times->s_s = times->s_s - (NTP_SECOND_OFFSET << 32);
	times->s_r = times->s_r - (NTP_SECOND_OFFSET << 32);
}

void printTimes(struct timeref_t *times) {
	printf("client send: %lu\n", times->c_s);
	printf("server recv: %lu\n", times->s_r);
	printf("server send: %lu\n", times->s_s);
	printf("client recv: %lu\n", times->c_r);
}

int main(int argc, char *argv[]) {
	if(argc <= 1) {
		fprintf(stderr, "arguments: hostname [, hostname ...]\n");
		return 1;
	}
	printf("#  - Server - - - - - Offset - - - - Delay\n");

	for (int i = 1; i < argc; i++) {
		ntp_payload_t *payload = (ntp_payload_t*) malloc(sizeof(ntp_payload_t));
		memset(payload, 0, sizeof(ntp_payload_t));

		payload->vn = 4;
		payload->mode = 3;

		struct timeref_t times;
		times = ntpRequest(argv[i], payload);
		times.s_r = (unsigned long)payload->receive_timestamp;
		times.s_s = (unsigned long)payload->transmit_timestamp;
		addOffsets(&times);
		//printTimes(&times);

		printf("%d - %s\t%lu\t%lu\n", i, argv[i], calculateOffset(&times) >> 32, calculateDelay(&times) >> 32);
		printf("\n");
	}
	return 0;
}

