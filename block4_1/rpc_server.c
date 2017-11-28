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

#define CMD_DEL 1
#define CMD_SET 2
#define CMD_GET 4
#define CMD_ACK 8
#define CMD_INL 128

unsigned int HEADER_SIZE_EXT = 6;
unsigned int HEADER_SIZE_INL = 14;

unsigned int BASE_PORT = 4000;
unsigned int HASH_SPACE = 100; // Maximum number of nodes in the ring

char *SELF_IP;
char *NEXT_IP;
char *SELF_PORT;
char *NEXT_PORT;
char *SELF_ID;
char *NEXT_ID;

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
    out_header[6] = (unsigned char) (in_header->id >> 8);
    out_header[7] = (unsigned char) (in_header->id % 256);
    out_header[8] = (unsigned char) (in_header->ip >> 24);
    out_header[9] = (unsigned char) (in_header->ip >> 16);
    out_header[10] = (unsigned char) (in_header->ip >> 8);
    out_header[11] = (unsigned char) (in_header->ip % 256);
    out_header[12] = (unsigned char) (in_header->port >> 8);
    out_header[13] = (unsigned char) (in_header->port % 256);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "arguments: id, next id\n");
        return 1;
    }

    SELF_ID = argv[1];
    NEXT_ID = argv[2];
    SELF_PORT = BASE_PORT + SELF_ID;
    NEXT_PORT = BASE_PORT + NEXT_ID;

    struct addrinfo hints, *res;
    int status;

    // Initialize Hashtagble of size 100
    init(100);

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
        char *request_ptr = (char *) request_header;
        memset(&request_header, 0, sizeof request_header);

        int read_size = sizeof request_header;

        ssize_t rs = 0;
        ssize_t read = 0;
        int twice = 0;

        header_t incoming_header;
        memset(&incoming_header, 0, sizeof(header_t));

        char *key_buffer = NULL;
        char *value_buffer = NULL;
        printf("[recv] Receiving Data\n");
        do {
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
        } while (rs > 0 && read < read_size);

        header_t outgoing_header;
        memset(&outgoing_header, 0, sizeof outgoing_header);

        int key_hash = hash(key_buffer, incoming_header.k_l) % HASH_SPACE;

        if (key_hash > (int) NEXT_ID) {

            // Forward to NEXT_ID instead

        } else {
            if (incoming_header.set) {
                printf("[recv] Received SET Command\n");
                outgoing_header.set = set(key_buffer, value_buffer, incoming_header.k_l, incoming_header.v_l);
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

        char h[6] = "000000";
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
