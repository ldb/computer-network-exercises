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
	clock_t send_time;
	clock_t receive_time;
};

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
	
	times.send_time = clock();
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
	times.receive_time = clock();

	char str[res->ai_addrlen];
	inet_ntop(AF_INET, &res->ai_addr, str, res->ai_addrlen);
	printf("%s\n", str);

	unmarshal(payload, &response[0]);
	freeaddrinfo(res);
	close(sockfd);

	return times;
}

float calculateDelay(struct timeref_t client, ntp_payload_t *server){ return 0.0;}
float calculateOffset(struct timeref_t client, ntp_payload_t *server){ return 0.0;}

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

		struct timeref_t client_delay;
		client_delay = ntpRequest(argv[i], test);

		printf("%d - %s\n", i, argv[i]);
		printPayload(test);
		printf("\n");
	}
	return 0;
}

