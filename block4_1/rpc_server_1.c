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
#include "marhsalling.h"
/*
#define CMD_DEL 1
#define CMD_SET 2
#define CMD_GET 4
#define CMD_ACK 8
#define CMD_INL 128

#define HEADER_SIZE_EXT 6
#define HEADER_SIZE_INL 14

unsigned int HASH_SPACE = 100; // Maximum number of nodes in the ring
unsigned int SELF_HASH_SPACE = 25;*/

char *SELF_IP;
char *NEXT_IP;
char *PREV_IP;
char *SELF_PORT;
char *NEXT_PORT;
char *PREV_PORT;
char *SELF_ID;
char *NEXT_ID;
char *PREV_ID;
/*
typedef struct header {
    unsigned int set : 1;
    unsigned int get : 1;
    unsigned int del : 1;
    unsigned int ack : 1;
    unsigned int inl : 1;
    unsigned int tid : 8;
    unsigned short k_l: 16;
    unsigned short v_l: 16;
    unsigned short id : 16;
    unsigned int ip : 32;
    unsigned short port: 16;
} header_t;

void unmarshal(header_t *out_header, unsigned char *in_header) {
    out_header->set = (unsigned int) (in_header[0] & CMD_SET) == CMD_SET;
    out_header->del = (unsigned int) (in_header[0] & CMD_DEL) == CMD_DEL;
    out_header->get = (unsigned int) (in_header[0] & CMD_GET) == CMD_GET;
    out_header->ack = (unsigned int) (in_header[0] & CMD_ACK) == CMD_ACK;
    out_header->inl = (unsigned int) (in_header[0] & CMD_INL) == CMD_INL;
    out_header->tid = (unsigned int) in_header[1];
    out_header->k_l = (unsigned short) (in_header[2] << 8);
    out_header->k_l += (unsigned short) (in_header[3]);
    out_header->v_l = (unsigned short) (in_header[4] << 8);
    out_header->v_l += (unsigned short) (in_header[5]);

    if (!out_header->inl) {
        return;
    }

    out_header->id = (unsigned short) (in_header[6] << 8);
    out_header->id += (unsigned short) (in_header[7]);
    out_header->ip = (unsigned int) (in_header[8] << 24);
    out_header->ip += (unsigned int) (in_header[9] << 16);
    out_header->ip += (unsigned int) (in_header[10] << 8);
    out_header->ip += (unsigned int) (in_header[11]);
    out_header->port = (unsigned short) (in_header[12] << 8);
    out_header->port += (unsigned short) (in_header[13]);
}

void marshal(char *out_header, header_t *in_header) {
    out_header[0] += (unsigned char) (in_header->set * CMD_SET);
    out_header[0] += (unsigned char) (in_header->del * CMD_DEL);
    out_header[0] += (unsigned char) in_header->get * CMD_GET;
    out_header[0] += (unsigned char) in_header->ack * CMD_ACK;
    out_header[1] = (unsigned char) in_header->tid;
    out_header[2] = (unsigned char) (in_header->k_l >> 8);
    out_header[3] = (unsigned char) (in_header->k_l % 256);
    out_header[4] = (unsigned char) (in_header->v_l >> 8);
    out_header[5] = (unsigned char) (in_header->v_l % 256);

    if (!in_header->inl) {
        return;
    }

    out_header[0] += (unsigned char) (in_header->inl * CMD_INL);
    out_header[6] = (unsigned char) (in_header->id >> 8);   //die ersten 8 Stellen der ID als MSB, erste Hälfte ins erste Byte der ID
    out_header[7] = (unsigned char) (in_header->id % 256);  //die letzten 8 Stellen der ID als LSB, zweite Hälfte ins zweite Byte der ID
    out_header[8] = (unsigned char) (in_header->ip >> 24);
    out_header[9] = (unsigned char) (in_header->ip >> 16);
    out_header[10] = (unsigned char) (in_header->ip >> 8);
    out_header[11] = (unsigned char) (in_header->ip % 256);
    out_header[12] = (unsigned char) (in_header->port >> 8);
    out_header[13] = (unsigned char) (in_header->port % 256);
}

void printHeader(header_t *header) {
    printf("set: %d\n", header->set);
    printf("get: %d\n", header->get);
    printf("del: %d\n", header->del);
    printf("ack: %d\n", header->ack);
    printf("tid: %d\n", header->tid);
    printf("inl: %d\n", header->inl);
    printf("id: %d\n", header->id);
    printf("ip %d\n", header->ip);
    printf("port: %d\n", header->port);
    printf("k_l: %d\n", header->k_l);
    printf("v_l: %d\n", header->k_l);
}
*/
void printBinary(char *binaryChar, int len) {
    for (int j = 0; j < len; j++) {
        for (int i = 0; i < 8; i++) {
            printf("%d", !!((binaryChar[j] << i) & 0x80));
        }
        printf(" %c\n", binaryChar[j]);
    }
}

int recv_all(header_t *incoming_header, int socket, unsigned char *request_header, char **key_buffer, char **value_buffer, int headersize) {
    char *request_ptr = (char *) request_header;
    int read_size = headersize;

    ssize_t rs = 0;
    int read = 0;
    int twice = 0;

    do {
        if ((rs = recv(socket, request_ptr, read_size, 0)) < 0) {
            fprintf(stderr, "recv: %s\n", strerror(errno));
            return 2;
        }
        read += rs;

        if (twice == 0) {
            twice++;
            unmarshal(incoming_header, &request_header[0]);
            //printHeader(incoming_header);
            read_size = incoming_header->k_l;
            printf("kl: %d\n", incoming_header->k_l);
            request_ptr = *key_buffer = malloc(read_size);
            read = 0;
            continue;
        }
        printf("rs: %zd\n", rs);

        if (read == incoming_header->k_l && incoming_header->v_l > 0 && twice == 1) {
            twice++;
            request_ptr = *value_buffer = malloc(incoming_header->v_l);
            read_size = incoming_header->v_l;
            printf("vl: %d\n", incoming_header->v_l);
            read = 0;
            continue;
        }

        request_ptr += rs;
    } while (rs > 0 && read < read_size);
    printf("done reading\n");
    return 0;
}

int sendToNextPeer(header_t *outgoing_header, char **key_buffer, char **value_buffer){
	printf("Peer Number %d sent to next peer number %d\n", atoi(SELF_ID), atoi(NEXT_ID));

    outgoing_header->inl = 1;
    outgoing_header->id = atoi(SELF_ID);
    outgoing_header->port = atoi(SELF_PORT);
    outgoing_header->ip = atoi(SELF_IP);

    printf("outgoing_header:\n");
    printHeader(outgoing_header);

    char h[HEADER_SIZE_INL] = "00000000000000";
	char *out_header = h;
	marshal(out_header, outgoing_header);

	size_t final_size = outgoing_header->k_l + outgoing_header->v_l + HEADER_SIZE_INL;
    char *outbuffer = malloc(final_size);

    memcpy(outbuffer, out_header, HEADER_SIZE_INL);
    memcpy(outbuffer + HEADER_SIZE_INL, key_buffer, outgoing_header->k_l);
    memcpy(outbuffer + HEADER_SIZE_INL + outgoing_header->k_l, value_buffer, outgoing_header->v_l);

    int to_send = final_size;

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

	do {
		int sent;
		if((sent = send(sockfd, outbuffer, to_send, 0)) == -1){
			fprintf(stderr, "send: %s\n", strerror(errno));
			return 2;
		}

		printf("[i] Sent %d bytes\n", sent);

		to_send -= sent;
		outbuffer += sent;
	} while (0 < to_send);
	outbuffer -= final_size;

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

    // Initialize Hashtagble of size 25
    init(SELF_HASH_SPACE);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // Do not specify IPv4 or IPv6 explicitely
    hints.ai_socktype = SOCK_STREAM; // Streaming socket protocol
    hints.ai_flags = AI_PASSIVE; // Use default local adress

    if ((status = getaddrinfo(NULL, SELF_PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    int sockfd;

    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        fprintf(stderr, "socket: %s\n", strerror(errno));
        return 2;
    }

    if ((bind(sockfd, res->ai_addr, res->ai_addrlen)) != 0) {
        fprintf(stderr, "bind: %s\n", strerror(errno));
        return 2;
    }

    if ((listen(sockfd, 1)) == -1) {
        fprintf(stderr, "listen: %s\n", strerror(errno));
        return 2;
    }

    while (1) {
        int temp_socket;
        int yes = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        struct sockaddr_storage incoming_addr;
        socklen_t addr_size = sizeof incoming_addr;
        temp_socket = accept(sockfd, (struct sockaddr *) &incoming_addr, &addr_size);

        printf("[acpt] New Connection\n");
        unsigned char request_header[HEADER_SIZE_EXT];
           //char *request_ptr = (char *) request_header;
        memset(&request_header, 0, sizeof request_header);

         /*   int read_size = sizeof request_header;

            ssize_t rs = 0;
            ssize_t read = 0;
            int twice = 0;*/

        header_t incoming_header;
        header_t *incoming_header_ptr = &incoming_header;
        memset(incoming_header_ptr, 0, sizeof(header_t));

        char *key_buffer = NULL;
        char *value_buffer = NULL;
        printf("[recv] Receiving Data\n");
       /*     do {
                if ((rs = recv(temp_socket, request_ptr, read_size, 0)) < 0) {
                    fprintf(stderr, "recv: %s\n", strerror(errno));
                    return 2;
                }
                read += rs;

                if (twice == 0) {
                    twice++;
                    unmarshal(&incoming_header, &request_header[0]);
                    read_size = incoming_header.k_l;
                    request_ptr = key_buffer = malloc(read_size);
                    read = 0;
                    continue;
                }

                if (read == incoming_header.k_l && incoming_header.v_l > 0 && twice == 1) {
                    twice++;
                    request_ptr = value_buffer = malloc(incoming_header.v_l);
                    read_size = incoming_header.v_l;
                    read = 0;
                    continue;
                }

                request_ptr += rs;
            } while (rs > 0 && read < read_size);*/

        recv_all(incoming_header_ptr, temp_socket, &request_header[0], &key_buffer, &value_buffer, sizeof request_header);

        header_t outgoing_header;
        memset(&outgoing_header, 0, sizeof outgoing_header);

        int key_hash = hash(key_buffer, incoming_header.k_l) % HASH_SPACE;

        if (key_hash >= (atoi(SELF_ID) - 1) + SELF_HASH_SPACE && !incoming_header.inl){
        	printf("OOOPS BOOOOY\n");
            sendToNextPeer(&outgoing_header, &key_buffer, &value_buffer);

        } else {
            if (incoming_header.set) {
                printf("[recv] Received SET Command\n");

                //printBinary(key_buffer, incoming_header.k_l);
                //printBinary(value_buffer, incoming_header.v_l);

                outgoing_header.set = set(key_buffer, value_buffer, (int)incoming_header.k_l, (int)incoming_header.v_l);
                outgoing_header.k_l = outgoing_header.v_l = 0;
            }

            if (incoming_header.get) {
                printf("[recv] Received GET Command\n");
                struct element *e;
                if ((e = get(key_buffer, incoming_header.k_l)) != NULL) {
                    value_buffer = malloc(e->valuelen);
                    memcpy(value_buffer, e->value, (size_t) e->valuelen);
                    outgoing_header.v_l = e->valuelen;
                    outgoing_header.get = 1;
                    outgoing_header.k_l = e->keylen;
                }
            }

            if (incoming_header.del) {
                printf("[recv] Received DEL Command\n");
                outgoing_header.del = del(key_buffer, incoming_header.k_l);
                outgoing_header.k_l = outgoing_header.v_l = 0;
            }

            outgoing_header.ack = 1;
            outgoing_header.tid = incoming_header.tid;
        }

        char h[HEADER_SIZE_EXT] = "000000";
        char *out_header = h;
        marshal(out_header, &outgoing_header);

        size_t final_size = outgoing_header.k_l + outgoing_header.v_l + HEADER_SIZE_EXT;
        char *outbuffer = malloc(final_size);

        memcpy(outbuffer, out_header, HEADER_SIZE_EXT);
        memcpy(outbuffer + HEADER_SIZE_EXT, key_buffer, outgoing_header.k_l);
        memcpy(outbuffer + HEADER_SIZE_EXT + outgoing_header.k_l, value_buffer, outgoing_header.v_l);

        int to_send = final_size;

        do {
            int sent = send(temp_socket, outbuffer, to_send, 0);
            to_send -= sent;
            outbuffer += sent;
        } while (to_send > 0);
        outbuffer -= final_size;

        printf("[send] Sent %zu bytes\n", final_size);

        close(temp_socket);

        printf("[send] Closed connection\n");

        free(key_buffer);
        free(value_buffer);
        free(outbuffer);

        memset(&request_header, 0, sizeof request_header);
        memset(&incoming_header, 0, sizeof incoming_header);
        memset(&outgoing_header, 0, sizeof outgoing_header);
    }

    close(sockfd);
    cleanup();
    return 0;
}
