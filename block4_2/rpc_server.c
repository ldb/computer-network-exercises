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

char *receiveKey(int key_len, int socket) {
	char *key_buffer = malloc(key_len);
	int rs = 0;

	if ((rs = recv(socket, key_buffer, key_len, 0)) < 0) {
		fprintf(stderr, "[%s][recv]: %s\n", SELF_ID, strerror(errno));
		return NULL;
	}

	return key_buffer;
}

char *receiveValue(int value_len, int socket) {
	if (value_len == 0) {
		return NULL;
	}

	char *value_buffer = malloc(value_len);
	int rs = 0;

	if ((rs = recv(socket, value_buffer, value_len, 0)) < 0) {
		fprintf(stderr, "[%s][recv]: %s\n", SELF_ID, strerror(errno));
		return NULL;
	}

	return value_buffer;
}

int requestFromNextPeer(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer, int temp_socket) {
	printf("[%s] request from peer %s\n", SELF_ID, NEXT_ID);

	if (incoming_header->inl == 0) {
		outgoing_header->id = atoi(SELF_ID);
		outgoing_header->port = atoi(SELF_PORT);
		outgoing_header->ip = atoi(SELF_IP);
		CLIENT_SOCKET = temp_socket;
	} else {
		outgoing_header->id = incoming_header->id;
		outgoing_header->ip = incoming_header->ip;
		outgoing_header->port = incoming_header->port;
	}

	outgoing_header->inl = 1;
	outgoing_header->ack = 0;
	outgoing_header->set = incoming_header->set;
	outgoing_header->get = incoming_header->get;
	outgoing_header->del = incoming_header->del;
	outgoing_header->tid = incoming_header->tid;
	outgoing_header->k_l = incoming_header->k_l;
	outgoing_header->v_l = incoming_header->v_l;

	char h[HEADER_SIZE_INL] = "00000000000000";
	char *out_header = h;
	marshal(out_header, outgoing_header);

	size_t final_size = outgoing_header->k_l + outgoing_header->v_l + HEADER_SIZE_INL;
	char *outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE_INL);
	memcpy(outbuffer + HEADER_SIZE_INL, key_buffer, outgoing_header->k_l);
	memcpy(outbuffer + HEADER_SIZE_INL + outgoing_header->k_l, value_buffer, outgoing_header->v_l);

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(NEXT_IP, NEXT_PORT, &hints, &res)) != 0) {
		fprintf(stderr, "[%s][getaddrinfo]: %s\n", SELF_ID, strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "[%s][connect]: %s\n", SELF_ID, strerror(errno));
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

	free(outbuffer);

	return 0;
}

int respondToPeer(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer) {
	printf("[%s] respond to peer %d\n", SELF_ID, incoming_header->id);

	outgoing_header->inl = 1;
	outgoing_header->tid = incoming_header->tid;
	outgoing_header->ack = 1;

	char h[HEADER_SIZE_INL] = "00000000000000";
	char *out_header = h;
	marshal(out_header, outgoing_header);

	size_t final_size = outgoing_header->k_l + outgoing_header->v_l + HEADER_SIZE_INL;
	char *outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE_INL);
	memcpy(outbuffer + HEADER_SIZE_INL, key_buffer, outgoing_header->k_l);
	memcpy(outbuffer + HEADER_SIZE_INL + outgoing_header->k_l, value_buffer, outgoing_header->v_l);

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	char tmp_PORT[5];
	snprintf(tmp_PORT, sizeof(tmp_PORT), "%d", incoming_header->port);

	if ((status = getaddrinfo(SELF_IP, &tmp_PORT[0], &hints, &res)) != 0) {
		fprintf(stderr, "[%s][getaddrinfo]: %s\n", SELF_ID, strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "[%s][connect]: %s\n", SELF_ID, strerror(errno));
		return 2;
	}

	int to_send = final_size;

	do {
		int sent;
		if ((sent = send(sockfd, outbuffer, to_send, 0)) == -1) {
			fprintf(stderr, "[%s][send]: %s\n", SELF_ID, strerror(errno));
			return 2;
		}

		printf("[%s] sent %d bytes\n", SELF_ID, sent);

		to_send -= sent;
		outbuffer += sent;
	} while (0 < to_send);
	outbuffer -= final_size;

	free(outbuffer);

	close(sockfd);

	return 0;
}

int respondToClient(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer, int temp_socket) {
	printf("[%s] respond to client\n", SELF_ID);

	if (incoming_header->inl == 1 && incoming_header->v_l != 0) {
		outgoing_header->get = 1;
		outgoing_header->v_l = incoming_header->v_l;
	}
	outgoing_header->k_l = incoming_header->k_l;
	outgoing_header->inl = 0;
	outgoing_header->tid = incoming_header->tid;
	outgoing_header->ack = 1;

	char h[HEADER_SIZE_EXT] = "000000";
	char *out_header = h;
	marshal(out_header, outgoing_header);

	size_t final_size = outgoing_header->k_l + outgoing_header->v_l + HEADER_SIZE_EXT;
	char *outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE_EXT);
	memcpy(outbuffer + HEADER_SIZE_EXT, key_buffer, outgoing_header->k_l);
	memcpy(outbuffer + HEADER_SIZE_EXT + outgoing_header->k_l, value_buffer, outgoing_header->v_l);

	int to_send = final_size;

	if (CLIENT_SOCKET != 0) {
		do {
			int sent = send(CLIENT_SOCKET, outbuffer, to_send, 0);
			to_send -= sent;
			outbuffer += sent;
		} while (to_send > 0);
		outbuffer -= final_size;
		close(CLIENT_SOCKET);
	}
	else {
		do {
			int sent = send(temp_socket, outbuffer, to_send, 0);
			to_send -= sent;
			outbuffer += sent;
		} while (to_send > 0);
		outbuffer -= final_size;
	}

	close(temp_socket);

	free(outbuffer);

	CLIENT_SOCKET = 0;
	return 0;
}

int stabilize(){
	header_t *outgoing_header = (header_t*)malloc(sizeof(header_t));
	memset(outgoing_header, 0, sizeof(header_t));

	outgoing_header->inl = 1;
	outgoing_header->stb = 1;

	char h[HEADER_SIZE_INL] = "00000000000000";
	char *out_header = h;
	marshal(out_header, outgoing_header);

	size_t final_size = HEADER_SIZE_INL;
	char *outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE_INL);

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(NEXT_IP, NEXT_PORT, &hints, &res)) != 0) {
		fprintf(stderr, "[%s][getaddrinfo]: %s\n", SELF_ID, strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "[%s][connect]: %s\n", SELF_ID, strerror(errno));
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

	return 0;
}

int notify(int socket){
	header_t *outgoing_header = (header_t*)malloc(sizeof(header_t));
	memset(outgoing_header, 0, sizeof(header_t));

	outgoing_header->inl = 1;
	outgoing_header->noy = 1;
	outgoing_header->id = atoi(PREV_ID);
	outgoing_header->ip = atoi(PREV_IP);
	outgoing_header->port = atoi(PREV_PORT);

	char h[HEADER_SIZE_INL] = "00000000000000";
	char *out_header = h;
	marshal(out_header, outgoing_header);

	size_t final_size = HEADER_SIZE_INL;
	char *outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE_INL);

	int to_send = final_size;

	do {
		int sent;
		if ((sent = send(socket, outbuffer, to_send, 0)) == -1) {
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
	close(socket);

	return 0;
}

int join(char *IpToCall, char *PortToCall){
	header_t *outgoing_header = (header_t*)malloc(sizeof(header_t));
	memset(outgoing_header, 0, sizeof(header_t));

	outgoing_header->inl = 1;
	outgoing_header->joi = 1;
	outgoing_header->id = atoi(SELF_ID);
	outgoing_header->ip = atoi(SELF_IP);
	outgoing_header->port = atoi(SELF_PORT);

	char h[HEADER_SIZE_INL] = "00000000000000";
	char *out_header = h;
	marshal(out_header, outgoing_header);

	size_t final_size = HEADER_SIZE_INL;
	char *outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE_INL);

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(IpToCall, PortToCall, &hints, &res)) != 0) {
		fprintf(stderr, "[%s][getaddrinfo]: %s\n", SELF_ID, strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "[%s][connect]: %s\n", SELF_ID, strerror(errno));
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

	return 0;
}

int main(int argc, char *argv[]) {
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
	int yes = 1;
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

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

	if(argc == 6){
		join(IpToCall, PortToCall);
		int temp_socket = accept(sockfd, (struct sockaddr *) &incoming_addr, &addr_size);
		header_t *incoming_header = receiveHeader(temp_socket);
		if(incoming_header->noy == 1){
			
		}
	}

	int test = 0;
	while (1) {
		int temp_socket;
		struct sockaddr_storage incoming_addr;
		socklen_t addr_size = sizeof incoming_addr;
		fd_set readfds;
		struct timeval tv;

		printf("Vor accept: %d\n", test);

		FD_ZERO(&readfds);

		FD_SET(sockfd, &readfds);

		tv.tv_sec = 4;
		tv.tv_usec = 200000;
		int rv = select(sockfd+1, &readfds, NULL, NULL, &tv);

		printf("Das kam raus: %d\n", rv);

		if(rv == -1){
			perror("select");
		}
		else if (rv == 0 && NEXT_ID != NULL){
			stabilize();
			temp_socket = accept(sockfd, (struct sockaddr *) &incoming_addr, &addr_size);
					
		}
		else if (FD_ISSET(sockfd, &readfds)){

			printf("[%s][acpt] New Connection\n", SELF_ID);

			header_t *incoming_header = receiveHeader(temp_socket);
			char *key_buffer = receiveKey(incoming_header->k_l, temp_socket);
			char *value_buffer = receiveValue(incoming_header->v_l, temp_socket);

			header_t *outgoing_header = (header_t*) malloc(sizeof(header_t));
			memset(outgoing_header, 0, sizeof(header_t));
			int key_hash = hash(key_buffer, incoming_header->k_l) % HASH_SPACE;

			// Operation was already completed by other peer, we should respond to client
			if (incoming_header->stb == 1){
				notify(temp_socket);
			} else if (incoming_header->inl == 1 && incoming_header->ack == 1) {
				respondToClient(outgoing_header, incoming_header, key_buffer, value_buffer, temp_socket);
			} else if (key_hash > atoi(SELF_ID) && key_hash < atoi(PREV_ID)) {			// CHECK AFTERWARDS
				requestFromNextPeer(outgoing_header, incoming_header, key_buffer, value_buffer, temp_socket);
			} else {
				if (incoming_header->set) {
					printf("[%s][recv] Received SET Command\n", SELF_ID);
					outgoing_header->set = set(key_buffer, value_buffer, (int)incoming_header->k_l, (int)incoming_header->v_l);
					outgoing_header->k_l = outgoing_header->v_l = 0;
				}

				if (incoming_header->get) {
					printf("[%s][recv] Received GET Command\n", SELF_ID);
					struct element *e;
					e = get(key_buffer, incoming_header->k_l);

					if (e != NULL) {
						value_buffer = malloc(e->valuelen);
						memcpy(value_buffer, e->value, (size_t) e->valuelen);
						outgoing_header->v_l = e->valuelen;
						outgoing_header->get = 1;
						outgoing_header->k_l = e->keylen;
					}
				}

				if (incoming_header->del) {
					printf("[%s][recv] Received DEL Command\n", SELF_ID);
					outgoing_header->del = del(key_buffer, incoming_header->k_l);
					outgoing_header->k_l = outgoing_header->v_l = 0;
				}

				outgoing_header->ack = 1;
				outgoing_header->tid = incoming_header->tid;

				if (incoming_header->inl == 1) {
					respondToPeer(outgoing_header, incoming_header, key_buffer, value_buffer);
				} else {
					respondToClient(outgoing_header, incoming_header, key_buffer, value_buffer, temp_socket);
					close(temp_socket);
				}
			}
			printf("[%s][send] Closed connection\n", SELF_ID);

			free(key_buffer);
			free(value_buffer);

			memset(&incoming_header, 0, sizeof incoming_header);
			memset(&outgoing_header, 0, sizeof outgoing_header);
		}
	}

	close(sockfd);
	cleanup();
	return 0;
}
