#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

// Parse URL to store Host and Path
void parseURL(char *url, char *fields[2]) {
	char *token;
	for (int i = 0; i < 3; i++) {
		token = strsep(&url, "/");
	}
	fields[0] = token;
	fields[1] = url;
}

// Generate an HTTP request with Host and Path
void generateRequest(char *fields[], char *request){
	strcpy(request, "GET /");
	strcat(request, fields[1]);
	strcat(request, " HTTP/1.1\r\nConnection: close\r\nHOST: ");
	strcat(request, fields[0]);
	strcat(request, "\r\n\r\n");
}

// Extract Content-Length from response header
int extractContentLength(char *response) {
	char *start = strstr(response, "Content-Length: ") + 16;
	char *end = strstr(start, "\n");

	size_t length = end - start;
    char *final = (char*)malloc(length + 1);
    strncpy(final, start, length);

    final[length] = '\0';
	return atoi(final);
}

// Seperate body from response header by slicing after \r\n\r\n and copy body to new buffer
void extractBody(char *body, char *response, int cl){
	char *start = strstr(response, "\r\n\r\n");
	if(start != NULL){
		start += 4;
		memcpy(body, start, cl);
	}
}

int main(int argc, char *argv[]) {
	if(argc != 2) {
		fprintf(stderr, "arguments: URL\n");
		return 1;
	}

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // Do not specify IPv4 or IPv6 explicitely
	hints.ai_socktype = SOCK_STREAM; // Streaming socket protocol

	// Array to store request information
	char *fields[2];
	parseURL(argv[1], fields);

	char request[512];
	generateRequest(fields, request);
	
	// Get adress info of server to connect to
	if((status = getaddrinfo(fields[0], "http", &hints, &res)) != 0) { // Use HTTP service
		fprintf(stderr, "getaddrinfo: %s\n", strerror(status));
		return 2;
	}

	int sockfd;

	// Create socket
	// We are connecting naively to the first IP we get, hoping that it works
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	
	if(connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "connect: %s\n", strerror(errno));
		return 2;
	}

	// Send request to server
	if(write(sockfd, request, strlen(request)) == -1){
		fprintf(stderr, "send: %s\n", strerror(errno));
		return 2;
	}

	// Declare Buffer for incoming Response, maximum size of the picture is 200kb
	char response[200000];
	memset(&response, 0, sizeof response);
	char *rptr = response;

	ssize_t resr = 0;
	int cl = 1;

	// Receive in a loop 
	do {
		if ((resr = recv(sockfd, rptr, sizeof(response), 0)) < 0) {
			fprintf(stderr, "recv: %s\n", strerror(errno));
			return 2;
		}

		if (cl == 1) { // Extract content length on first incoming message
			cl = extractContentLength(&response[0]);
		}

		rptr += resr; // Increase buffer pointer to not overwrite data
	} while (resr > 0);

	// New buffer to store extracted body
	char body[cl];
	memset(&body, 0, cl);
	extractBody(&body[0], &response[0], cl);

	fwrite(body, 1, cl, stdout);

	// cleanup
	freeaddrinfo(res);
	close(sockfd);

	return 0;
}
