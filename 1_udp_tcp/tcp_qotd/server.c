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

void getRandomQuote(char *message, char *file) {
	// File to be opened in read-mode
	FILE *fp;
	fp = fopen(file, "r");

	int lines = 0; // Lines counter
 
	// Go through whole file and count lines
	while(!feof(fp)){
		if(fgetc(fp) == '\n'){
			lines++;
		}
	}

	lines++; // Last line does not have a newline, so was not counted yet

	// Get random line number
	srand(time(NULL)); 
	int randomLine = rand() % lines + 1; // Get a random number between 0 and line count

	// Jump to beginning of file and load the random line
	rewind(fp);	 
	for(int i = 0; i <= randomLine; i++){
		fgets(message, 512, fp); // Maximum size of line according to RFC 865
	}

	fclose(fp);
	return;
}

int main(int argc, char *argv[]) {	
	if(argc != 3) {
		fprintf(stderr, "arguments: port, filename\n");
		return 1;
	}

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // Do not specify IPv4 or IPv6 explicitely
	hints.ai_socktype = SOCK_STREAM; // Streaming socket protocol
	hints.ai_flags = AI_PASSIVE; // Use default local adress

	// Get adress info for our host
	if((status = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 2;
	}

	int sockfd;

	// Create socket
	if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
		fprintf(stderr, "socket: %s\n", strerror(errno));
		return 2;
	}

	// Bind to socket using our own adress
	if((bind(sockfd, res->ai_addr, res->ai_addrlen)) != 0) {
		fprintf(stderr, "bind: %s\n", strerror(errno));
		return 2;
	}
	
	// Set connection queue size to 1 as we only want one connection at a time
	if((listen(sockfd, 1)) == -1) {
		fprintf(stderr, "listen: %s\n", strerror(errno));
		return 2;
	}

	while(1) {
		int temp_socket;
		struct sockaddr_storage incoming_addr;
		socklen_t addr_size = sizeof(incoming_addr);

		// New Socket for new incoming connection
		temp_socket = accept(sockfd, (struct sockaddr *)&incoming_addr, &addr_size);

		// Get a random line from a given text file
		char msg[512];
		getRandomQuote(&msg[0], argv[2]);
		int len = strlen(msg);

		// Send message to connection and close the connection
		send(temp_socket, msg, len, 0);
		close(temp_socket);
	}

	close(sockfd);
	return 0;
}
