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
#include <pthread.h>
#include "hashtable.h"
#include "rpc_server.h"
#include "marshalling.h"

#define HEADER_SIZE_EXT 6
#define HEADER_SIZE_INL 14
#define HASH_SPACE 100
#define SELF_HASH_SPACE 25

char *SELF_IP;
char *NEXT_IP;
char *PREV_IP;
char *SELF_PORT;
char *NEXT_PORT;
char *PREV_PORT;
char *SELF_ID;
char *NEXT_ID;
char *PREV_ID;
int CLIENT_SOCKET; // Save socket information of client
int STILL_BUILDING;

void printHeader(header_t *header){
	printf("Set: %d\n", header->set);
	printf("Get: %d\n", header->get);
	printf("Del: %d\n", header->del);
	printf("Ack: %d\n", header->ack);
	printf("Stb: %d\n", header->stb);
	printf("Noy: %d\n", header->noy);
	printf("Joi: %d\n", header->joi);
	printf("Inl: %d\n", header->inl);
	printf("Tid: %d\n", header->tid);
	printf("Key_length: %d\n", header->k_l);
	printf("Value_length: %d\n", header->v_l);
	printf("ID: %d\n", header->id);
	printf("IP: %d\n", header->ip);
	printf("PORT: %d\n", header->port);
}

header_t *receiveHeader(int socket) {
	unsigned char request_header[HEADER_SIZE_EXT];
	int read_size = HEADER_SIZE_EXT;
	int rs = 0;
	header_t *incoming_header = (header_t*)malloc(sizeof(header_t));
	memset(incoming_header, 0, sizeof(header_t));

	if ((rs = recv(socket, request_header, read_size, 0)) < 0) {
		fprintf(stderr, "[%s][recv]: %s\n", SELF_ID, strerror(errno));
		return NULL;
	}

	unmarshal(incoming_header, &request_header[0]);

	if (incoming_header->inl == 1) {
		unsigned char overflow[8];
		unsigned char new_request_header[HEADER_SIZE_INL];

		if ((rs = recv(socket, overflow, 8, 0)) < 0) {
			fprintf(stderr, "[%s][recv]: %s\n", SELF_ID, strerror(errno));
			return NULL;
		}

		memcpy(new_request_header, request_header, HEADER_SIZE_EXT);
		memcpy(new_request_header + HEADER_SIZE_EXT, overflow, 8);
		header_t *incoming_header_inl = (header_t*)malloc(sizeof(header_t));
		memset(incoming_header_inl, 0, sizeof(header_t));
		unmarshal(incoming_header_inl, &new_request_header[0]);

		free(incoming_header);

		return incoming_header_inl;
	}
	return incoming_header;
}

int sendToNextPeer1(header_t *incoming_header){
	unsigned char h[HEADER_SIZE_INL] = "00000000000000";
	unsigned char *out_header = h;
	memset(out_header, 0, sizeof(*out_header));
	marshal(out_header, incoming_header);

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(NEXT_IP, NEXT_PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	
	if(connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "connect: %s\n", strerror(errno));
		return 2;
	}

	int sent;
	if((sent = send(sockfd, out_header, HEADER_SIZE_INL, 0)) == -1){
		fprintf(stderr, "send: %s\n", strerror(errno));
		return 2;
	}

	close(sockfd);

	return 0;
}

int stabilize(){
	header_t *outgoing_header = (header_t *) malloc(sizeof(header_t));
	memset(outgoing_header, 0, sizeof(header_t));

	outgoing_header->inl = 1;
	outgoing_header->stb = 1;
	outgoing_header->id = atoi(SELF_ID);
	outgoing_header->ip = atoi(SELF_IP);
	outgoing_header->port = atoi(SELF_PORT);

	unsigned char h[HEADER_SIZE_INL] = "00000000000000";
	unsigned char *out_header = h;
	memset(out_header, 0, HEADER_SIZE_INL);
	marshal(out_header, outgoing_header);

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(NEXT_IP, NEXT_PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	
	if(connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "connect: %s\n", strerror(errno));
		return 2;
	}

	printf("sending stabilize!\n");

	int sent;
	if((sent = send(sockfd, out_header, HEADER_SIZE_INL, 0)) == -1){
		fprintf(stderr, "send: %s\n", strerror(errno));
		return 2;
	}

	close(sockfd);
 
	return 0;
}

int notify(header_t *incoming_header){
	header_t *outgoing_header = (header_t *) malloc(sizeof(header_t));
	memset(outgoing_header, 0, sizeof(header_t));

	if (PREV_ID == NULL){
		PREV_ID = malloc(sizeof(char) * 2);
		PREV_IP = malloc(sizeof(char) * 4);
		PREV_PORT = malloc(sizeof(char) * 2);
		snprintf(PREV_ID, 16, "%d", incoming_header->id);
		snprintf(PREV_IP, 32, "%d", incoming_header->ip);
		snprintf(PREV_PORT, 16, "%d", incoming_header->port);
		if(NEXT_ID == NULL){
			NEXT_ID = malloc(sizeof(char) * 2);
			NEXT_IP = malloc(sizeof(char) * 4);
			NEXT_PORT = malloc(sizeof(char) * 2);
			snprintf(NEXT_ID, 16, "%d", incoming_header->id);
			snprintf(NEXT_IP, 32, "%d", incoming_header->ip);
			snprintf(NEXT_PORT, 16, "%d", incoming_header->port);
		}
	}
	else if (atoi(PREV_ID) != incoming_header->id && incoming_header->joi == 1){
		memset(PREV_ID, 0, sizeof(*PREV_ID));
		memset(PREV_IP, 0, sizeof(*PREV_IP));
		memset(PREV_PORT, 0, sizeof(*PREV_PORT));
		snprintf(PREV_ID, 16, "%d", incoming_header->id);
		snprintf(PREV_IP, 32, "%d", incoming_header->ip);
		snprintf(PREV_PORT, 16, "%d", incoming_header->port);
	}

	if(incoming_header->joi == 1){
		outgoing_header->inl = 1;
		outgoing_header->noy = 1;
		outgoing_header->stb = 0;
		outgoing_header->joi = 0;
		outgoing_header->id = atoi(SELF_ID);
		outgoing_header->ip = atoi(SELF_IP);
		outgoing_header->port = atoi(SELF_PORT);
	}
	else if (incoming_header->stb == 1){
		outgoing_header->inl = 1;
		outgoing_header->noy = 1;
		outgoing_header->stb = 0;
		outgoing_header->id = atoi(PREV_ID);
		outgoing_header->ip = atoi(PREV_IP);
		outgoing_header->port = atoi(PREV_PORT);
	}
	else {
		printf("I should not be here...\n");
	}

	printf("print header:\n");
	printHeader(outgoing_header);

	unsigned char h[HEADER_SIZE_INL] = "00000000000000";
	unsigned char *out_header = h;
	marshal(out_header, outgoing_header);

	size_t final_size = HEADER_SIZE_INL;
	unsigned char *outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE_INL);

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	char tmp_IP[4];
	char tmp_PORT[2];

	snprintf(tmp_IP, 32, "%d", incoming_header->id);
	snprintf(tmp_PORT, 16, "%d", incoming_header->port);

	if((status = getaddrinfo(tmp_IP, tmp_PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	
	if(connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "connect: %s\n", strerror(errno));
		return 2;
	}

	int to_send = final_size;

	do {
		int sent;
		if ((sent = send(sockfd, outbuffer, to_send, 0)) == -1) {
			fprintf(stderr, "[%s][send] %s\n", SELF_ID, strerror(errno));
			return 2;
		}

		printf("[%s] sent %d bytes\n", SELF_ID, sent);

		to_send -= sent;
		outbuffer += sent;
	} while (0 < to_send);
	outbuffer -= final_size;

	free(outgoing_header);
	free(outbuffer);
	close(sockfd);

	return 0;
}

int join(char *IpToCall, char *PortToCall){
	header_t *outgoing_header = (header_t *) malloc(sizeof(header_t));
	memset(outgoing_header, 0, sizeof(header_t));

	outgoing_header->inl = 1;
	outgoing_header->joi = 1;
	outgoing_header->id = atoi(SELF_ID);
	outgoing_header->ip = atoi(SELF_IP);
	outgoing_header->port = atoi(SELF_PORT);

	printf("will send header for join:\n");
	printHeader(outgoing_header);

	unsigned char h[HEADER_SIZE_INL] = "00000000000000";
	unsigned char *out_header = h;
	memset(out_header, 0, HEADER_SIZE_INL);
	marshal(out_header, outgoing_header);

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(IpToCall, PortToCall, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	
	if(connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "connect: %s\n", strerror(errno));
		return 2;
	}

	int sent;
	if((sent = send(sockfd, out_header, HEADER_SIZE_INL, 0)) == -1){
		fprintf(stderr, "send: %s\n", strerror(errno));
		return 2;
	}

	printf("[i] Sent %d bytes\n", sent);

	close(sockfd);

	return 0;
}

void *thread(void *arg){
	while(STILL_BUILDING == 1){
		sleep(2);
		if(NEXT_ID != NULL){
			stabilize();
		}
	}

	return NULL;
}

int main(int argc, char *argv[]){
	if(argc != 3 && argc != 4 && argc != 6){
		printf("How to use:\nFor Base Node: ./server IP PORT [ID]\nFor new Node: ./server IP PORT ID BASE_IP BASE_PORT\n");
		return 0;
	}

	char *IpToCall;
	char *PortToCall;
	
	if(argc == 3 || argc == 4){
		SELF_IP = argv[1];
		SELF_PORT = argv[2];
		SELF_ID = "0";
		if(argc == 4){
			SELF_ID = argv[3];
		}
		PREV_ID = NULL;
		PREV_PORT = NULL;
		PREV_IP = NULL;
		NEXT_ID = NULL;
		NEXT_PORT = NULL;
		NEXT_IP = NULL;
		IpToCall = NULL;
		PortToCall = NULL;

		printf("Peer Number %s will act as Base!\n", SELF_ID);

	}

	if(argc == 6){
		SELF_IP = argv[1];
		SELF_PORT = argv[2];
		SELF_ID = argv[3];
		IpToCall = argv[4];
		PortToCall = argv[5];
		join(IpToCall, PortToCall);
	}

	struct addrinfo hints, *res;
	int status;

	init(SELF_HASH_SPACE); // Initialize Hashtagble of size 25 (1TABLESIZE divided by 4 because we have 4 peers)

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // Do not specify IPv4 or IPv6 explicitely
	hints.ai_socktype = SOCK_STREAM; // Streaming socket protocol
	hints.ai_flags = AI_PASSIVE; // Use default local adress

	if ((status = getaddrinfo(NULL, SELF_PORT, &hints, &res)) != 0) {
		fprintf(stderr, "[%s][getaddrinfo]: %s\n", SELF_ID, strerror(status));
		return 2;
	}

	int sockfd;

	if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
		fprintf(stderr, "[%s][socket]: %s\n", SELF_ID, strerror(errno));
		return 2;
	}

	if ((bind(sockfd, res->ai_addr, res->ai_addrlen)) != 0) {
		fprintf(stderr, "[%s][bind]: %s\n", SELF_ID, strerror(errno));
		return 2;
	}

	if ((listen(sockfd, 1)) == -1) {
		fprintf(stderr, "[%s][listen]: %s\n", SELF_ID, strerror(errno));
		return 2;
	}

	printf("Created my own Socket on Peer Number %s!\n", SELF_ID);
	fd_set readset;
	FD_ZERO(&readset);
	//struct timeval tv;
	//tv.tv_sec = 2;
	//tv.tv_usec = 0;
	STILL_BUILDING = 1;
	pthread_t pt;
	pthread_create(&pt, NULL, thread, NULL);

	while (1) {
		int temp_socket;
		struct sockaddr_storage incoming_addr;
		socklen_t addr_size = sizeof incoming_addr;
		 
		temp_socket = accept(sockfd, (struct sockaddr *) &incoming_addr, &addr_size);
		printf("[%s][acpt] New Connection\n", SELF_ID);

		header_t *incoming_header = receiveHeader(temp_socket);

		printf("received Header:\n");
		printHeader(incoming_header);

		header_t *outgoing_header = (header_t*) malloc(sizeof(header_t));
		memset(outgoing_header, 0, sizeof(header_t));

		// Operation was already completed by other peer, we should respond to client
		if (incoming_header->stb == 1){
			printf("received Stabilize!\n");
			notify(incoming_header);
		}
		else if (incoming_header->noy == 1){
			printf("received Notify!\n");
			if(NEXT_ID == NULL){
				NEXT_ID = malloc(sizeof(char) * 2);
				NEXT_IP = malloc(sizeof(char) * 4);
				NEXT_PORT = malloc(sizeof(char) * 2);
				snprintf(NEXT_ID, 16, "%d", incoming_header->id);
				snprintf(NEXT_IP, 32, "%d", incoming_header->ip);
				snprintf(NEXT_PORT, 16, "%d", incoming_header->port);
			}
			else if(incoming_header->id != atoi(SELF_ID)){
				memset(NEXT_ID, 0, sizeof(*NEXT_ID));
				memset(NEXT_IP, 0, sizeof(*NEXT_IP));
				memset(NEXT_PORT, 0, sizeof(*NEXT_PORT));
				snprintf(NEXT_ID, 16, "%d", incoming_header->id);
				snprintf(NEXT_IP, 32, "%d", incoming_header->ip);
				snprintf(NEXT_PORT, 16, "%d", incoming_header->port);
			}
		}
		else if (incoming_header->joi == 1){
			printf("received Join\n");
			if(NEXT_ID != NULL && PREV_ID != NULL && atoi(PREV_ID) > atoi(SELF_ID)){
				if(incoming_header->id < atoi(PREV_ID) && incoming_header->id > atoi(SELF_ID)){
					sendToNextPeer1(incoming_header);
				}
				else{
					notify(incoming_header);
				}
				//sendToNextPeer1(incoming_header);
			}
			else if(NEXT_ID != NULL && incoming_header->id > atoi(SELF_ID)){
				sendToNextPeer1(incoming_header);
			}
			else {
				notify(incoming_header);
			}
		}
		else if (incoming_header->set == 1){
			STILL_BUILDING = 0;
		}

		close(temp_socket);
		printf("[%s][send] Closed connection\n", SELF_ID);

		printf("MEINE DATEN ALS NODE NUMMER %s:\n", SELF_ID);
		printf("NEXT_ID: %s\tNEXT_PORT: %s\n", NEXT_ID, NEXT_PORT);
		printf("PREV_ID: %s\tPREV_PORT: %s\n", PREV_ID, PREV_PORT);

		memset(&incoming_header, 0, sizeof incoming_header);
		memset(&outgoing_header, 0, sizeof outgoing_header);
			
		
	}

	close(sockfd);
	cleanup();

	return 0;
}