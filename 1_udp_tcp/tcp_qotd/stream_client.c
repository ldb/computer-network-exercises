#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "arguments: hostname port\n");
		return 1;
	}

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", strerror(status));
		return 2;
	}

	int sockfd;

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	clock_t start_t = clock();

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "connect: %s\n", strerror(errno));
		return 2;
	}

	char buf[513];
	int resr;

	if ((resr = recv(sockfd, buf, 512, MSG_WAITALL)) <= 0) {
		fprintf(stderr, "recv: %s\n", strerror(errno));
		return 2;
	}

	buf[resr] = '\0';
	clock_t end_t = clock();

	printf("Time: %Lf\nResponse: %s", (long double)(end_t - start_t), buf);

	freeaddrinfo(res);
	close(sockfd);

	return 0;
}
