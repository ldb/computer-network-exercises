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

long double ntpRequest(char *host, ntp_payload_t *payload) {
	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if((status = getaddrinfo(host, NTP_PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", strerror(status));
		return 2;
	}

	int sockfd;
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	unsigned char msg[48];
	marshal(&msg[0], payload);
	
	clock_t start_t = clock();
	if(sendto(sockfd, msg, sizeof msg, 0, res->ai_addr, res->ai_addrlen) <= 0) {
		fprintf(stderr, "recv: %s\n", strerror(errno));
		return 2;
	}

	unsigned char response[48];

	if((recvfrom(sockfd, response, 48, MSG_WAITALL, res->ai_addr, &res->ai_addrlen)) <= 0) {
		fprintf(stderr, "recv: %s\n", strerror(errno));
		return 2;
	}

	unmarshal(payload, &response[0]);
	freeaddrinfo(res);
	close(sockfd);

	return clock() - start_t;
}

int main(int argc, char *argv[]) {
	if(argc <= 1) {
		fprintf(stderr, "arguments: hostname [, hostname ...]\n");
		return 1;
	}

	for (int i = 1; i < argc; i++) {
		ntp_payload_t *test = (ntp_payload_t*) malloc(sizeof(ntp_payload_t));
		memset(test, 0, sizeof(ntp_payload_t));

		test->vn = 4;
		test->mode = 3;

		long double time = ntpRequest(argv[i], test);
		printf("%d %s response time: %Lf\n", i, argv[i], time);
		printPayload(test);
	}
	return 0;
}

