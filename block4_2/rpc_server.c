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
#define STBZ_INTERVAL 5

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

int sendData(int socked, char *buffer, int length) {
	int to_send = length;

	do {
		int sent;
		if ((sent = send(socked, buffer, to_send, 0)) == -1) {
			fprintf(stderr, "![%s][sendData][send]: %s\n", SELF_ID, strerror(errno));
			return 2;
		}

		//printf("[%s][sendData] sent %d bytes\n", SELF_ID, sent);

		to_send -= sent;
		buffer += sent;
	} while (0 < to_send);
	buffer -= length;
	return 0;
}

int prepareBuffer(char **outbuffer, header_t *outgoing_header, int headerSize, char *key_buffer, char *value_buffer) {

	unsigned char h[headerSize];
	unsigned char *out_header = h;
	memset(&out_header, 0, headerSize);

	marshal(out_header, outgoing_header);

	size_t final_size = outgoing_header->k_l + outgoing_header->v_l + HEADER_SIZE_INT;
	*outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE_INT);
	memcpy(outbuffer + HEADER_SIZE_INT, key_buffer, outgoing_header->k_l);
	memcpy(outbuffer + HEADER_SIZE_INT + outgoing_header->k_l, value_buffer, outgoing_header->v_l);

	return final_size;
}

int requestFromNextPeer(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer, int temp_socket) {
	printf("[%s] request from peer %s\n", SELF_ID, NEXT_ID);

	if (!incoming_header->intl) {
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
	outgoing_header->join = incoming_header->join;
	incoming_header->noti = incoming_header->noti;
	incoming_header->stbz = incoming_header->stbz;
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

	// char *outbuffer = NULL;
	// int final_size = prepareBuffer(*(outbuffer), outgoing_header, HEADER_SIZE_INT, key_buffer, value_buffer);

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(NEXT_IP, NEXT_PORT, &hints, &res)) != 0) {
		fprintf(stderr, "![%s][requestFromNextPeer][getaddrinfo]: %s\n", SELF_ID, strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "![%s][requestFromNextPeer][connect]: %s\n", SELF_ID, strerror(errno));
		return 2;
	}

	sendData(sockfd, outbuffer, final_size);

	//free(outbuffer);

	return 0;
}

int respondToPeer(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer) {
	printf("[%s] respond to peer %d\n", SELF_ID, incoming_header->id);

	outgoing_header->intl = 1;
	outgoing_header->tid = incoming_header->tid;
	//outgoing_header->ack = 1;

	unsigned char h[HEADER_SIZE_INT] = "00000000000000";
	unsigned char *out_header = h;
	marshal(out_header, outgoing_header);

	size_t final_size = outgoing_header->k_l + outgoing_header->v_l + HEADER_SIZE_INT;
	char *outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE_INT);
	memcpy(outbuffer + HEADER_SIZE_INT, key_buffer, outgoing_header->k_l);
	memcpy(outbuffer + HEADER_SIZE_INT + outgoing_header->k_l, value_buffer, outgoing_header->v_l);

	// char *outbuffer = NULL;
	// int final_size = prepareBuffer(*(outbuffer), outgoing_header, HEADER_SIZE_INT, key_buffer, value_buffer);

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	char tmp_PORT[5];
	snprintf(tmp_PORT, sizeof(tmp_PORT), "%d", incoming_header->port);

	if ((status = getaddrinfo(SELF_IP, &tmp_PORT[0], &hints, &res)) != 0) {
		fprintf(stderr, "![%s][respondToPeer][getaddrinfo]: %s\n", SELF_ID, strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "![%s][respondToPeer][connect]: %s\n", SELF_ID, strerror(errno));
		return 2;
	}

	sendData(sockfd, outbuffer, final_size);

	//free(outbuffer);

	close(sockfd);

	return 0;
}

int respondToClient(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer, int temp_socket) {
	printf("[%s] respond to client\n", SELF_ID);

	if (incoming_header->intl && incoming_header->v_l) {
		outgoing_header->get = 1;
		outgoing_header->v_l = incoming_header->v_l;
	}
	outgoing_header->k_l = incoming_header->k_l;
	outgoing_header->intl = 0;
	outgoing_header->tid = incoming_header->tid;
	outgoing_header->ack = 0;

	unsigned char h[HEADER_SIZE_EXT] = "000000";
	unsigned char *out_header = h;
	marshal(out_header, outgoing_header);

	size_t final_size = outgoing_header->k_l + outgoing_header->v_l + HEADER_SIZE_EXT;
	char *outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE_EXT);
	memcpy(outbuffer + HEADER_SIZE_EXT, key_buffer, outgoing_header->k_l);
	memcpy(outbuffer + HEADER_SIZE_EXT + outgoing_header->k_l, value_buffer, outgoing_header->v_l);

	// char *outbuffer = NULL;
	// int final_size = prepareBuffer(*(outbuffer), outgoing_header, HEADER_SIZE_EXT, key_buffer, value_buffer);

	if (CLIENT_SOCKET) {
		sendData(CLIENT_SOCKET, outbuffer, final_size);
		close(CLIENT_SOCKET);
	} else {
		sendData(temp_socket, outbuffer, final_size);
		close(temp_socket);
	}

	//free(outbuffer);

	CLIENT_SOCKET = 0;
	return 0;
}

int join() {
	header_t *request_header = (header_t*) malloc(sizeof(header_t));
	memset(request_header, 0, sizeof(header_t));

	request_header->intl = 1;
	request_header->join = 1;
	request_header->id = atoi(SELF_ID);
	request_header->ip = atoi(SELF_IP);
	request_header->port = atoi(SELF_PORT);

	unsigned char h[HEADER_SIZE_INT];
	unsigned char *out_header = h;
	memset(out_header, 0, HEADER_SIZE_INT);

	marshal(out_header, request_header);

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(NEXT_IP, NEXT_PORT, &hints, &res)) != 0) {
		fprintf(stderr, "![%s][join][getaddrinfo]: %s\n", SELF_ID, strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "![%s][join][connect]: %s\n", SELF_ID, strerror(errno));
		return 2;
	}

	sendData(sockfd, (char*)out_header, HEADER_SIZE_INT);
	close(sockfd);
	return 0;

}

int notify(char *rcIP, char *rcPORT, char *ID, char *IP, char *PORT) {
	header_t *request_header = (header_t*) malloc(sizeof(header_t));
	memset(request_header, 0, sizeof(header_t));

	request_header->intl = 1;
	request_header->noti = 1;
	request_header->id = atoi(ID);
	request_header->ip = atoi(IP);
	request_header->port = atoi(PORT);

	unsigned char h[HEADER_SIZE_INT];
	unsigned char *out_header = h;
	memset(out_header, 0, HEADER_SIZE_INT);

	marshal(out_header, request_header);

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(rcIP, rcPORT, &hints, &res)) != 0) {
		fprintf(stderr, "![%s][notify][getaddrinfo]: %s\n", SELF_ID, strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "![%s][notify][connect]: %s\n", SELF_ID, strerror(errno));
		return 2;
	}

	sendData(sockfd, (char*)out_header, HEADER_SIZE_INT);
	close(sockfd);
	return 0;
}

int stabilize() {
	header_t *request_header = (header_t*) malloc(sizeof(header_t));
	memset(request_header, 0, sizeof(header_t));

	request_header->intl = 1;
	request_header->stbz = 1;
	request_header->id = atoi(SELF_ID);
	request_header->ip = atoi(SELF_IP);
	request_header->port = atoi(SELF_PORT);

	unsigned char h[HEADER_SIZE_INT];
	unsigned char *out_header = h;
	memset(out_header, 0, HEADER_SIZE_INT);

	marshal(out_header, request_header);

	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(NEXT_IP, NEXT_PORT, &hints, &res)) != 0) {
		fprintf(stderr, "![%s][stabilize][getaddrinfo]: %s\n", SELF_ID, strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "![%s][stabilize][connect]: %s\n", SELF_ID, strerror(errno));
		return 2;
	}

	sendData(sockfd, (char*)out_header, HEADER_SIZE_INT);
	close(sockfd);
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc != 4 && argc != 7) {
		fprintf(stderr, "arguments: SELF_ID, SELF_IP, SELF_PORT [, PEER_ID, PEER_IP, PEER_PORT]\n");
		return 1;
	}

	SELF_ID = argv[1];
	SELF_IP = argv[2];
	SELF_PORT = argv[3];

	if (argc == 7) {
		NEXT_ID = argv[4];
		NEXT_IP = argv[5];
		NEXT_PORT = argv[6];
		// PREV_ID = strdup(argv[1]);
		// PREV_IP = strdup(argv[2]);
		// PREV_PORT = strdup(argv[3]);
		// // memset(PREV_IP, 0, sizeof(*argv[2]));
		// memset(PREV_ID, 0, sizeof(*PREV_ID));
		// // memset(PREV_PORT, 0, sizeof(*argv[3]));
	}

	// We are alone :(
	if (!NEXT_ID) {
		NEXT_ID = strdup(argv[1]);
		PREV_ID = strdup(argv[1]);
		NEXT_IP = strdup(argv[2]);
		PREV_IP = strdup(argv[2]);
		NEXT_PORT = strdup(argv[3]);
		PREV_PORT = strdup(argv[3]);
	}

	struct addrinfo hints, *res;
	int status;

	init(HASH_SPACE);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(NULL, SELF_PORT, &hints, &res)) != 0) {
		fprintf(stderr, "![%s][main][getaddrinfo]: %s\n", SELF_ID, strerror(status));
		return 2;
	}

	int sockfd;
	int yes = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
		fprintf(stderr, "![%s][main][socket]: %s\n", SELF_ID, strerror(errno));
		return 2;
	}

	if ((bind(sockfd, res->ai_addr, res->ai_addrlen)) != 0) {
		fprintf(stderr, "![%s][main][bind]: %s\n", SELF_ID, strerror(errno));
		return 2;
	}

	if ((listen(sockfd, 1)) == -1) {
		fprintf(stderr, "![%s][main][listen]: %s\n", SELF_ID, strerror(errno));
		return 2;
	}

	printf("[%s] Coming online!\n", SELF_ID);

	if (!PREV_ID) {
		printf("[%s] Trying to JOIN!\n", SELF_ID);
		join();
	}

	int cnt = 1;
	int firstSet = 0;

	// fd_set readset;
	// FD_ZERO(&readset);
	// struct timeval tv;
	// tv.tv_sec = 5;

	while (1) {
		cnt++;

		//FD_SET(sockfd, &readset);

    	//select(sockfd + 1, &readset, NULL, NULL, &tv);

		if (atoi(SELF_ID) != atoi(NEXT_ID)) {
		printf("[%s] Sending STABILIZE to %s\n", SELF_ID, NEXT_ID);

		// 	// if (!PREV_ID) {
		// 	// 	PREV_ID = strdup(NEXT_ID);
		// 	// }
		//sleep(rand() % );
		stabilize();
		//continue;
		}

		// if (atoi(SELF_ID) != atoi(PREV_ID) && atoi(SELF_ID) == atoi(NEXT_ID)) {
		// 	NEXT_ID = strdup(PREV_ID);
		// }

		printf("[%s] Tick: %d PREV: %s, NEXT: %s\n", SELF_ID, cnt, PREV_ID, NEXT_ID);

		int temp_socket;
		struct sockaddr_storage incoming_addr;
		socklen_t addr_size = sizeof incoming_addr;
		temp_socket = accept(sockfd, (struct sockaddr *) &incoming_addr, &addr_size);

		//printf("[%s][main][acpt] New Connection\n", SELF_ID);

		unsigned char request_header[HEADER_SIZE_INT];
		char *request_ptr = (char *) request_header;
		memset(&request_header, 0, sizeof request_header);

		int read_size = HEADER_SIZE_EXT;

		ssize_t rs = 0;
		ssize_t read = 0;
		int thrice = 0;

		header_t incoming_header;
		header_t *incoming_header_ptr = &incoming_header;
		memset(incoming_header_ptr, 0, sizeof(header_t));

		char *key_buffer = NULL;
		char *value_buffer = NULL;
		//printf("[%s][main][recv] Receiving Data\n", SELF_ID);

		do {
			if ((rs = recv(temp_socket, request_ptr, read_size, 0)) < 0) {
				fprintf(stderr, "![%s][main][recv]: %s\n", SELF_ID, strerror(errno));
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
		//printf("[%s][main][hash] Key hash: %d\n", SELF_ID, key_hash);

		if (key_hash > atoi(SELF_ID) && PREV_ID && atoi(SELF_ID) > atoi(PREV_ID)) {

			//requestFromNextPeer(outgoing_header, &incoming_header, key_buffer, value_buffer, temp_socket);

		} else if (firstSet && !incoming_header.intl) {
			if (incoming_header.set) {
				printf("[%s][main][recv] Received SET Command\n", SELF_ID);
				outgoing_header->set = set(key_buffer, value_buffer, (int)incoming_header.k_l, (int)incoming_header.v_l);
				outgoing_header->k_l = outgoing_header->v_l = 0;
			}

			if (incoming_header.get) {
				printf("[%s][main][recv] Received GET Command\n", SELF_ID);
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
				printf("[%s][main][recv] Received DEL Command\n", SELF_ID);
				outgoing_header->del = del(key_buffer, incoming_header.k_l);
				outgoing_header->k_l = outgoing_header->v_l = 0;
			}

			outgoing_header->ack = 1;
			outgoing_header->tid = incoming_header.tid;
			close(temp_socket);
			//printf("[%s][recv][join] Closed connection\n", SELF_ID);
			continue;

		} else {
			// Operation was already completed by other peer, we should respond to client
			// if (incoming_header.intl && incoming_header.ack) {
			// 	respondToClient(outgoing_header, &incoming_header, key_buffer, value_buffer, temp_socket);
			// }

			if (incoming_header.intl && incoming_header.join) {
				printf("[%s][main][recv] Received JOIN Command\n", SELF_ID);

				int temp_next = atoi(SELF_ID) < atoi(NEXT_ID) ? atoi(NEXT_ID) : atoi(NEXT_ID) + HASH_SPACE;
				if (atoi(SELF_ID) != atoi(NEXT_ID) && ((incoming_header.id > atoi(SELF_ID) && incoming_header.id < temp_next) || (incoming_header.id < atoi(PREV_ID) && incoming_header.id < atoi(SELF_ID)))) {
					printf("[%s][main][recv] Forward JOIN Command\n", SELF_ID);
					//requestFromNextPeer(outgoing_header, &incoming_header, key_buffer, value_buffer, temp_socket);
				} else {
					*PREV_IP = (char)incoming_header.ip;
					snprintf(PREV_ID, sizeof(PREV_ID), "%d", incoming_header.id);
					snprintf(PREV_PORT, sizeof(PREV_PORT), "%d", incoming_header.port);
					printf("[%s][recv][join] Updated PREV: %s, Sending NOTIFY to %s\n", SELF_ID, PREV_ID, PREV_ID);

					if (atoi(SELF_ID) == atoi(NEXT_ID)) {
						*NEXT_IP = (char)incoming_header.ip;
						snprintf(NEXT_ID, sizeof(NEXT_ID), "%d", incoming_header.id);
						snprintf(NEXT_PORT, sizeof(NEXT_PORT), "%d", incoming_header.port);
						printf("[%s][recv][join] Also updated NEXT: %s\n", SELF_ID, NEXT_ID);
					}
					notify(PREV_IP, PREV_PORT, PREV_ID, PREV_IP, PREV_PORT);
				}
				close(temp_socket);
				//printf("[%s][recv][join] Closed connection\n", SELF_ID);
				continue;
			}

			if (incoming_header.intl && incoming_header.noti) {
				printf("[%s][main][recv] Received NOTIFY Command\n", SELF_ID);

				if (incoming_header.id != atoi(SELF_ID)) {
					*NEXT_IP = (char)incoming_header.ip;
					snprintf(NEXT_ID, sizeof(NEXT_ID), "%d", incoming_header.id);
					snprintf(NEXT_PORT, sizeof(NEXT_PORT), "%d", incoming_header.port);
					printf("[%s][recv][noti] Updated NEXT: %s\n", SELF_ID, NEXT_ID);
				}

				// if (PREV_ID) {
				// 	//notify();
				// 	printf("[%s][recv][noti] Sending NOTIFY to: %s\n", SELF_ID, PREV_ID);
				// }

				close(temp_socket);
				//printf("[%s][recv][noti] Closed connection\n", SELF_ID);
				continue;
			}

			if (incoming_header.intl && incoming_header.stbz) {
				printf("[%s][main][recv] Received STABILIZE Command\n", SELF_ID);

				if (!PREV_ID) {
					// Initializing only
					PREV_IP = strdup(SELF_IP);
					PREV_ID = strdup(SELF_ID);
					PREV_PORT = strdup(SELF_PORT);

					*PREV_IP = (char)incoming_header.ip;
					snprintf(PREV_ID, sizeof(PREV_ID), "%d", incoming_header.id);
					snprintf(PREV_PORT, sizeof(PREV_PORT), "%d", incoming_header.port);
					printf("[%s][recv][stbz] Updated PREV: %s\n", SELF_ID, PREV_ID);
					close(temp_socket);
					continue;

				} else {
					outgoing_header->id = atoi(PREV_ID);
					outgoing_header->ip = atoi(PREV_IP);
					outgoing_header->port = atoi(PREV_PORT);
					outgoing_header->noti = 1;
					printf("[%s][recv][stbz] Responding with PREV %s\n", SELF_ID, PREV_ID);
					//respondToPeer(outgoing_header, &incoming_header, key_buffer, value_buffer);

					char *FROM_IP = strdup(SELF_IP);
					char *FROM_PORT = strdup(SELF_PORT);

					snprintf(FROM_IP, sizeof(FROM_IP), "%d", incoming_header.ip);
					snprintf(FROM_PORT, sizeof(FROM_PORT), "%d", incoming_header.port);

					//srand(time(NULL)); 
					notify(FROM_IP,FROM_PORT,PREV_ID, PREV_IP, PREV_PORT);
					close(temp_socket);
					//	printf("[%s][recv][stbz] Closed connection\n", SELF_ID);
					continue;
				}
				//stabilize();
				//requestFromNextPeer(outgoing_header, &incoming_header, key_buffer, value_buffer, temp_socket);
			}
			// if (incoming_header.intl) {
			// 	//respondToPeer(outgoing_header, &incoming_header, key_buffer, value_buffer);
			// } else {
			// 	respondToClient(outgoing_header, &incoming_header, key_buffer, value_buffer, temp_socket);
			// 	close(temp_socket);
			// }
			// printf("[%s][main][send] Closed connection\n", SELF_ID);
		}
	}

	close(sockfd);
	cleanup();
	return 0;
}
