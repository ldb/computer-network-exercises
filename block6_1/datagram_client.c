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

typedef struct timeref_t {
	int err;
	unsigned long c_s;
	unsigned long c_r;
	unsigned long s_r;
	unsigned long s_s;
} timeref_t;

typedef struct times_t {
	double c_s;
	double c_r;
	double s_r;
	double s_s;
} times_t;

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

	if ((status = getaddrinfo(host, NTP_PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", strerror(status));
		times.err = -1;
		return times;
	}

	int sockfd;
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	unsigned char msg[48];
	payload->vn = 4;
	payload->mode = 3;
	marshal(&msg[0], payload);

	times.c_s = getTime();

	if (sendto(sockfd, msg, sizeof msg, 0, res->ai_addr, res->ai_addrlen) <= 0) {
		fprintf(stderr, "recv: %s\n", strerror(errno));
		times.err = -1;
		return times;
	}

	unsigned char response[48];

	if ((recvfrom(sockfd, response, 48, MSG_WAITALL, res->ai_addr, &res->ai_addrlen)) <= 0) {
		fprintf(stderr, "recv: %s\n", strerror(errno));
		times.err = -1;
		return times;
	}

	times.c_r = getTime();

	unmarshal(payload, &response[0]);
	times.s_r = (unsigned long)payload->receive_timestamp;
	times.s_s = (unsigned long)payload->transmit_timestamp;

	freeaddrinfo(res);
	close(sockfd);

	return times;
}

double calculateOffset(struct times_t *times) {
	return ((times->s_r - times->c_s) + (times->s_s - times->c_r)) / 2;
}

double calculateDelay(struct times_t *times) {
	return ((times->c_r - times->c_s) - (times->s_s - times->s_r));
}

double normalizeTime(unsigned long time, int ntp) {
	double s = 0.0 + (time >> 32);
	double ms = (double)((time << 32) >> 32) / (ntp ? 1e10 : 1e3);
	return s + ms;
}

void addUnixOffsets(struct timeref_t *times) {
	times->s_s = times->s_s - (NTP_SECOND_OFFSET << 32);
	times->s_r = times->s_r - (NTP_SECOND_OFFSET << 32);
}

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		fprintf(stderr, "arguments: hostname [, hostname ...]\n");
		return 1;
	}

	int bestServer = 0;
	double bestDelay = 1;
	double bestOffset = 0;

	printf("  | Server                                      | Stratum  | Offset  | Delay    |\n");
	printf("---------------------------------------------------------------------------------\n");

	for (int i = 1; i < argc; i++) {
		ntp_payload_t *payload = (ntp_payload_t*) malloc(sizeof(ntp_payload_t));
		memset(payload, 0, sizeof(ntp_payload_t));

		struct timeref_t times = ntpRequest(argv[i], payload);
		addUnixOffsets(&times);

		struct times_t seconds = {
			.c_s = normalizeTime(times.c_s, 0),
			.c_r = normalizeTime(times.c_r, 0),
			.s_r = normalizeTime(times.s_r, 1),
			.s_s = normalizeTime(times.s_s, 1)
		};

		double delay = calculateDelay(&seconds);
		double offset = calculateOffset(&seconds);

		if (delay < bestDelay) {
			bestServer = i;
			bestOffset = offset;
		}

		printf("%2d|%-45s|%10d|%9f|%10f|\n", i, argv[i], payload->stratum, offset, delay);
	}

	printf("\nBest Server based on delay: %d: %s\n", bestServer, argv[bestServer]);
	printf("Current clock (%f) should be adjusted by %f seconds\n", normalizeTime(getTime(), 0), bestOffset);
	return 0;
}

