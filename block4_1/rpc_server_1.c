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
#define HASH_SPACE 25
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

    printf("read_size: %d\n", read_size);

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
            printf("KEYBUFFER SIZE: %d\n", incoming_header->k_l);
            printf("kl: %d\n", incoming_header->k_l);
            request_ptr = *key_buffer = malloc(read_size);
            read = 0;
            continue;
        }
        printf("rs: %zd\n", rs);

        if (read == incoming_header->k_l && incoming_header->v_l > 0 && twice == 1) {
            twice++;
            request_ptr = *value_buffer = malloc(incoming_header->v_l-1);
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

int robertReceive(header_t *incoming_header, int socket, unsigned char *request_header, char *key_buffer, char *value_buffer, int headersize){
	char *request_ptr = (char *) request_header;
	int rs = 0;
    int read_size = headersize;
    printf("1read_size: %d\n", read_size);

    if ((rs = recv(socket, request_ptr, read_size, 0)) < 0) {
        fprintf(stderr, "recv: %s\n", strerror(errno));
        return 2;
    }

    unmarshal(incoming_header, &request_header[0]);
    /*

    read_size = incoming_header->k_l;
    char *tmp_buffer = (char*)malloc(read_size*sizeof(char));
    key_buffer = (char*)malloc(read_size*sizeof(char)+1);

    if ((rs = recv(socket, tmp_buffer, read_size, 0)) < 0) {
        fprintf(stderr, "recv: %s\n", strerror(errno));
        return 2;
    }

    memcpy(key_buffer, tmp_buffer, read_size);

    printf("key_buffer: %s\n", key_buffer);

    free(tmp_buffer);

    key_buffer[read_size] = '\0';

    read_size = incoming_header->v_l;
    tmp_buffer = (char*)malloc(read_size*sizeof(char));
    value_buffer = (char*)malloc(read_size*sizeof(char)+1);
    printf("2read_size: %d\n", read_size);

    if ((rs = recv(socket, tmp_buffer, read_size, 0)) < 0) {
        fprintf(stderr, "recv: %s\n", strerror(errno));
        printf("is there an error?\n");
        return 2;
    }

    memcpy(value_buffer, tmp_buffer, read_size);

    value_buffer[read_size] = '\0';
*/
    return 0;
}

header_t *receiveHeader(int socket){
	unsigned char request_header[HEADER_SIZE_EXT];
	int read_size = HEADER_SIZE_EXT;
	int rs = 0;
	header_t *incoming_header = (header_t*)malloc(sizeof(header_t));

	if ((rs = recv(socket, request_header, read_size, 0)) < 0) {
        fprintf(stderr, "recv: %s\n", strerror(errno));
        return NULL;
    }

    unmarshal(incoming_header, &request_header[0]);

	return incoming_header;
}

char *receiveKey(int key_len, int socket){
	char *key_buffer = malloc(key_len+1);
	int rs = 0;

	if ((rs = recv(socket, key_buffer, key_len, 0)) < 0) {
        fprintf(stderr, "recv: %s\n", strerror(errno));
        return NULL;
    }

    key_buffer[key_len] = '\0';

	return key_buffer;
}

char *receiveValue(int value_len, int socket){
	char *value_buffer = malloc(value_len+1);
	int rs = 0;

	if ((rs = recv(socket, value_buffer, value_len, 0)) < 0) {
        fprintf(stderr, "recv: %s\n", strerror(errno));
        return NULL;
    }

    value_buffer[value_len] = '\0';

	return value_buffer;
}


int sendToNextPeer(header_t *outgoing_header, char *key_buffer, char *value_buffer){
	printf("Peer Number %d sent to next peer number %d\n", atoi(SELF_ID), atoi(NEXT_ID));

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

int sendToFirstPeer(header_t *outgoing_header, char *key_buffer, char *value_buffer){
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

        //header_t *incoming_header_ptr = &incoming_header;
        //memset(incoming_header_ptr, 0, sizeof(header_t));

        //char *key_buffer = NULL;
        //char *value_buffer = NULL;
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

        header_t *incoming_header = receiveHeader(temp_socket);
        char *key_buffer = receiveKey(incoming_header->k_l, temp_socket);
        char *value_buffer = receiveValue(incoming_header->v_l, temp_socket);

        //printBinary(key_buffer, incoming_header.k_l);
        //printf("keybuffer: %s\n", key_buffer);

        header_t *outgoing_header = (header_t*)malloc(sizeof(header_t));
        memset(outgoing_header, 0, sizeof(header_t));
        printf("key_buffer: %s\n", key_buffer);
        int key_hash = hash(key_buffer, incoming_header->k_l) % HASH_SPACE;

        if (key_hash >= (atoi(SELF_ID) - 1) + SELF_HASH_SPACE && !incoming_header->inl){
        	printf("will send it to the next peer firstly\n");
        	outgoing_header->inl = 1;
		    outgoing_header->id = atoi(SELF_ID);
		    outgoing_header->port = atoi(SELF_PORT);
		    outgoing_header->ip = atoi(SELF_IP);

            sendToNextPeer(outgoing_header, key_buffer, value_buffer);
        }
       	else if(key_hash >= (atoi(SELF_ID) - 1) + SELF_HASH_SPACE && incoming_header->inl){
       		printf("will send it to the next peer\n");
       		sendToNextPeer(outgoing_header, key_buffer, value_buffer);
       	}
       	else if(incoming_header->inl){
       		printf("gotcha\n");
       		sendToFirstPeer(outgoing_header, key_buffer, value_buffer);
       	}
        else {
        	printf("can handle it on my own!\n");
            if (incoming_header->set) {
                printf("[recv] Received SET Command\n");
                printf("what i can print:\n");
                printf("key_buffer: %s\n", key_buffer);
                printf("value_buffer: %s\n", value_buffer);
                printf("key_length: %d\n", (int)incoming_header->k_l);
                printf("value_length: %d\n", (int)incoming_header->v_l);

                //printBinary(key_buffer, incoming_header.k_l);
                //printBinary(value_buffer, incoming_header.v_l);
                outgoing_header->set = set(key_buffer, value_buffer, (int)incoming_header->k_l, (int)incoming_header->v_l);
                printf("did the set\n");
                outgoing_header->k_l = outgoing_header->v_l = 0;
            }

            if (incoming_header->get) {
                printf("[recv] Received GET Command\n");
                struct element *e;
                if ((e = get(key_buffer, incoming_header->k_l)) != NULL) {
                    value_buffer = malloc(e->valuelen);
                    memcpy(value_buffer, e->value, (size_t) e->valuelen);
                    outgoing_header->v_l = e->valuelen;
                    outgoing_header->get = 1;
                    outgoing_header->k_l = e->keylen;
                }
            }

            if (incoming_header->del) {
                printf("[recv] Received DEL Command\n");
                outgoing_header->del = del(key_buffer, incoming_header->k_l);
                outgoing_header->k_l = outgoing_header->v_l = 0;
            }

            outgoing_header->ack = 1;
            outgoing_header->tid = incoming_header->tid;
        }

        printf("finished function!\n");

        char h[HEADER_SIZE_EXT] = "000000";
        char *out_header = h;
        marshal(out_header, outgoing_header);

        size_t final_size = outgoing_header->k_l + outgoing_header->v_l + HEADER_SIZE_EXT;
        char *outbuffer = malloc(final_size);

        memcpy(outbuffer, out_header, HEADER_SIZE_EXT);
        memcpy(outbuffer + HEADER_SIZE_EXT, key_buffer, outgoing_header->k_l);
        memcpy(outbuffer + HEADER_SIZE_EXT + outgoing_header->k_l, value_buffer, outgoing_header->v_l);

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
