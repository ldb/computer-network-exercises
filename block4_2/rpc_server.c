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
#define HEADER_SIZE_INT 14
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

int requestFromNextPeer(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer, int temp_socket) {
	printf("[%s] request from peer %s\n", SELF_ID, NEXT_ID);

	if (incoming_header->intl == 0) {
		outgoing_header->id = atoi(SELF_ID);
		outgoing_header->port = atoi(SELF_PORT);
		outgoing_header->ip = atoi(SELF_IP);
		CLIENT_SOCKET = temp_socket;
	} else {
		outgoing_header->id = incoming_header->id;
		outgoing_header->ip = incoming_header->ip;
		outgoing_header->port = incoming_header->port;
	}

	outgoing_header->intl = 1;
	outgoing_header->ack = 0;
	outgoing_header->set = incoming_header->set;
	outgoing_header->get = incoming_header->get;
	outgoing_header->del = incoming_header->del;
	outgoing_header->tid = incoming_header->tid;
	outgoing_header->k_l = incoming_header->k_l;
	outgoing_header->v_l = incoming_header->v_l;

	unsigned char h[HEADER_SIZE_INT] = "00000000000000";
	unsigned char *out_header = h;
	marshal(out_header, outgoing_header);

	size_t final_size = outgoing_header->k_l + outgoing_header->v_l + HEADER_SIZE_INT;
	char *outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE_INT);
	memcpy(outbuffer + HEADER_SIZE_INT, key_buffer, outgoing_header->k_l);
	memcpy(outbuffer + HEADER_SIZE_INT + outgoing_header->k_l, value_buffer, outgoing_header->v_l);

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

	//free(outbuffer);

	return 0;
}

int respondToPeer(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer) {
	printf("[%s] respond to peer %d\n", SELF_ID, incoming_header->id);

	outgoing_header->intl = 1;
	outgoing_header->tid = incoming_header->tid;
	outgoing_header->ack = 1;

	unsigned char h[HEADER_SIZE_INT] = "00000000000000";
	unsigned char *out_header = h;
	marshal(out_header, outgoing_header);

	size_t final_size = outgoing_header->k_l + outgoing_header->v_l + HEADER_SIZE_INT;
	char *outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE_INT);
	memcpy(outbuffer + HEADER_SIZE_INT, key_buffer, outgoing_header->k_l);
	memcpy(outbuffer + HEADER_SIZE_INT + outgoing_header->k_l, value_buffer, outgoing_header->v_l);

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

	//free(outbuffer);

	close(sockfd);

	return 0;
}

int respondToClient(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer, int temp_socket) {
	printf("[%s] respond to client\n", SELF_ID);

	if (incoming_header->intl == 1 && incoming_header->v_l != 0) {
		outgoing_header->get = 1;
		outgoing_header->v_l = incoming_header->v_l;
	}
	outgoing_header->k_l = incoming_header->k_l;
	outgoing_header->intl = 0;
	outgoing_header->tid = incoming_header->tid;
	outgoing_header->ack = 1;

	unsigned char h[HEADER_SIZE_EXT] = "000000";
	unsigned char *out_header = h;
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

	//free(outbuffer);

	CLIENT_SOCKET = 0;
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc != 8) {
		fprintf(stderr, "arguments: port, id, previous port, previous id, next port, next id, ip-adress\n");
		return 1;
	}

	SELF_PORT = argv[1];
	NEXT_PORT = argv[5];
	PREV_PORT = argv[3];
	SELF_ID = argv[2];
	NEXT_ID = argv[6];
	PREV_ID = argv[4];
	SELF_IP = argv[7];
	NEXT_IP = argv[7];
	PREV_IP = argv[7];

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

	while (1) {
		int temp_socket;
		struct sockaddr_storage incoming_addr;
		socklen_t addr_size = sizeof incoming_addr;
		temp_socket = accept(sockfd, (struct sockaddr *) &incoming_addr, &addr_size);

		printf("[%s][acpt] New Connection\n", SELF_ID);

		unsigned char request_header[HEADER_SIZE_INT];
		char *request_ptr = (char *) request_header;
		memset(&request_header, 0, sizeof request_header);
		//request_header[0] = request_header[0] >> 8;

		printBinary(&request_header[0], HEADER_SIZE_INT);

		int read_size = HEADER_SIZE_EXT;

		ssize_t rs = 0;
		ssize_t read = 0;
		int thrice = 0;

		header_t incoming_header;
		header_t *incoming_header_ptr = &incoming_header;
		memset(incoming_header_ptr, 0, sizeof(header_t));

		char *key_buffer = NULL;
		char *value_buffer = NULL;
		printf("[%s][recv] Receiving Data\n", SELF_ID);

		do {
			if ((rs = recv(temp_socket, request_ptr, read_size, 0)) < 0) {
				fprintf(stderr, "recv: %s\n", strerror(errno));
				return 2;
			}
			read += rs;

			if (thrice == 0) {
				unmarshal(&incoming_header, &request_header[0]);
				read = 0;

				if (incoming_header.intl == 1) {
					read_size = HEADER_SIZE_INT - HEADER_SIZE_EXT;
					request_ptr += rs;
					thrice++;
					continue;
				}

				read_size = incoming_header.k_l;
				request_ptr = key_buffer = malloc(read_size);
				thrice = 2;
				continue;
			}

			if (thrice == 1) {
				unmarshal(&incoming_header, &request_header[0]);
				read_size = incoming_header.k_l;
				request_ptr = key_buffer = malloc(read_size);
				thrice++;
				read = 0;

				printBinary(&request_header[0], HEADER_SIZE_INT);
				//printHeader(&incoming_header);

				continue;
			}

			if (thrice == 2 && read == incoming_header.k_l && incoming_header.v_l > 0) {
				thrice++;
				request_ptr = value_buffer = malloc(incoming_header.v_l);
				read_size = incoming_header.v_l;
				read = 0;
				continue;
			}

			request_ptr += rs;
		} while (rs > 0 && read < read_size);

		header_t *outgoing_header = (header_t*) malloc(sizeof(header_t));
		memset(outgoing_header, 0, sizeof(header_t));
		int key_hash = hash(key_buffer, incoming_header.k_l) % HASH_SPACE;

		// Operation was already completed by other peer, we should respond to client
		if (incoming_header.intl == 1 && incoming_header.ack == 1) {
			respondToClient(outgoing_header, &incoming_header, key_buffer, value_buffer, temp_socket);
		} else if (key_hash > (atoi(SELF_ID) + SELF_HASH_SPACE)) {
			requestFromNextPeer(outgoing_header, &incoming_header, key_buffer, value_buffer, temp_socket);
		} else {
			if (incoming_header.set) {
				printf("[%s][recv] Received SET Command\n", SELF_ID);
				outgoing_header->set = set(key_buffer, value_buffer, (int)incoming_header.k_l, (int)incoming_header.v_l);
				outgoing_header->k_l = outgoing_header->v_l = 0;
			}

			if (incoming_header.get) {
				printf("[%s][recv] Received GET Command\n", SELF_ID);
				struct element *e;
				e = get(key_buffer, incoming_header.k_l);

				if (e != NULL) {
					value_buffer = malloc(e->valuelen);
					memcpy(value_buffer, e->value, (size_t) e->valuelen);
					outgoing_header->v_l = e->valuelen;
					outgoing_header->get = 1;
					outgoing_header->k_l = e->keylen;
				}
			}

			if (incoming_header.del) {
				printf("[%s][recv] Received DEL Command\n", SELF_ID);
				outgoing_header->del = del(key_buffer, incoming_header.k_l);
				outgoing_header->k_l = outgoing_header->v_l = 0;
			}

			outgoing_header->ack = 1;
			outgoing_header->tid = incoming_header.tid;

			if (incoming_header.intl == 1) {
				respondToPeer(outgoing_header, &incoming_header, key_buffer, value_buffer);
			} else {
				respondToClient(outgoing_header, &incoming_header, key_buffer, value_buffer, temp_socket);
				close(temp_socket);
			}
		}
		printf("[%s][send] Closed connection\n", SELF_ID);

		//free(key_buffer);
		//free(value_buffer);

		memset(&incoming_header, 0, sizeof incoming_header);
		memset(&outgoing_header, 0, sizeof outgoing_header);
	}

	close(sockfd);
	cleanup();
	return 0;
}
