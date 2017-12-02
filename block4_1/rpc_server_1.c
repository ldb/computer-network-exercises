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

// Information about the next and previous Peers
char *SELF_IP;
char *NEXT_IP;
char *PREV_IP;
char *SELF_PORT;
char *NEXT_PORT;
char *PREV_PORT;
char *SELF_ID;
char *NEXT_ID;
char *PREV_ID;
// Saves the socket information of a client
int CLIENT_SOCKET;

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
    printf("v_l: %d\n", header->v_l);
}

void printBinary(char *binaryChar, int len) {
    for (int j = 0; j < len; j++) {
        for (int i = 0; i < 8; i++) {
            printf("%d", !!((binaryChar[j] << i) & 0x80));
        }
        printf(" %c\n", binaryChar[j]);
    }
}

// Receives and returns a header
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

    // check if it was an external or internal header
    // if it was internal, have to receive more
    // also return then the internal header
    if(incoming_header->inl == 1){
    	unsigned char overflow[8];
    	unsigned char new_request_header[HEADER_SIZE_INL];

    	if ((rs = recv(socket, overflow, 8, 0)) < 0) {
        fprintf(stderr, "recv: %s\n", strerror(errno));
        return NULL;
    	}

    	memcpy(new_request_header, request_header, HEADER_SIZE_EXT);
    	memcpy(new_request_header + HEADER_SIZE_EXT, overflow, 8);
    	header_t *incoming_header_inl = (header_t*)malloc(sizeof(header_t));
    	unmarshal(incoming_header_inl, &new_request_header[0]);

    	free(incoming_header);

    	return incoming_header_inl;
    }
	return incoming_header;
}

// receives and returns the key
char *receiveKey(int key_len, int socket){
	char *key_buffer = malloc(key_len);
	int rs = 0;

	if ((rs = recv(socket, key_buffer, key_len, 0)) < 0) {
        fprintf(stderr, "recv: %s\n", strerror(errno));
        return NULL;
    }

	return key_buffer;
}

// receives and returns the value
char *receiveValue(int value_len, int socket){
	if(value_len == 0){
		return NULL;
	}

	char *value_buffer = malloc(value_len);
	int rs = 0;

	if ((rs = recv(socket, value_buffer, value_len, 0)) < 0) {
        fprintf(stderr, "recv: %s\n", strerror(errno));
        return NULL;
    }

	return value_buffer;
}

// Send the information to the next peer in the chord
// returns 0 when succeeded
// returns 2 when an error occures
int sendToNextPeer(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer, int temp_socket){
	printf("Peer Number %d calls peer number %d\n", atoi(SELF_ID), atoi(NEXT_ID));

    if(incoming_header->inl == 0){
	    outgoing_header->id = atoi(SELF_ID);
	    outgoing_header->port = atoi(SELF_PORT);
	    outgoing_header->ip = atoi(SELF_IP);
	    CLIENT_SOCKET = temp_socket;
	}
	else{
		outgoing_header->id = incoming_header->id;
		outgoing_header->ip = incoming_header->ip;
		outgoing_header->port = incoming_header->port;
	}

	outgoing_header->inl = 1;
	outgoing_header->k_l = incoming_header->k_l;
	outgoing_header->v_l = incoming_header->v_l;
	outgoing_header->tid = incoming_header->tid;
	outgoing_header->ack = 0;
	outgoing_header->set = incoming_header->set;
	outgoing_header->get = incoming_header->get;
	outgoing_header->del = incoming_header->del;

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

	free(outbuffer);

	return 0;
}

// Send the information back to the peer, who started the call
// returns 0 when succeeded
// returns 2 when an error occures
int sendInfosBack(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer){
	printf("Peer %s will send the infos back at Peer %d\n", SELF_ID, incoming_header->id);

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

    int to_send = final_size;

    struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	char tmp_PORT[4];
	sprintf(tmp_PORT, "%d", incoming_header->port);

	if((status = getaddrinfo(SELF_IP, tmp_PORT, &hints, &res)) != 0) {
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

	free(outbuffer);

	close(sockfd);

	return 0;
}

// sends information back the the client
// returns 0 when succeeded
// returns no errors because we are good programmers
int sendInfosToClient(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer, int temp_socket){
	// when the get function was successful (value length is not zero) and internal, set the get bit to 1
	// when the get function was unsuccessful and internal, let it on 0
	// when the get function was external, get is already correctly set
	if(incoming_header->inl == 1 && incoming_header->v_l != 0){
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

    if(CLIENT_SOCKET != 0){
    	do {
            int sent = send(CLIENT_SOCKET, outbuffer, to_send, 0);
            to_send -= sent;
            outbuffer += sent;
        } while (to_send > 0);
        outbuffer -= final_size;
        close(CLIENT_SOCKET);
    }
    else{
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
    SELF_IP = argv[7];			// IP is always the same
    NEXT_IP = argv[7];
    PREV_IP = argv[7];

    struct addrinfo hints, *res;
    int status;

    // Initialize Hashtagble of size 25 (1TABLESIZE divided by 4 because we have 4 peers)
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

        header_t *incoming_header = receiveHeader(temp_socket);
        char *key_buffer = receiveKey(incoming_header->k_l, temp_socket);
        char *value_buffer = receiveValue(incoming_header->v_l, temp_socket);

        header_t *outgoing_header = (header_t*)malloc(sizeof(header_t));
        memset(outgoing_header, 0, sizeof(header_t));
        int key_hash = hash(key_buffer, incoming_header->k_l) % HASH_SPACE;

        // When this peer was called internal but the operation was already done
        // this means this peer has called the operation first, so let the client
        // know that its operation was done
        if(incoming_header->inl == 1 && incoming_header->ack == 1){
        	sendInfosToClient(outgoing_header, incoming_header, key_buffer, value_buffer, temp_socket);
        }
        // When the hash key is not in range of this peers index
        // then send the operation forward to the next peer in the chord
        else if(key_hash >= ((atoi(SELF_ID) - 1) + SELF_HASH_SPACE)){
            sendToNextPeer(outgoing_header, incoming_header, key_buffer, value_buffer, temp_socket);
        }
        // otherwise, this peer is responsible for the operation
        else{
            if (incoming_header->set) {
                printf("[recv] Received SET Command\n");
                outgoing_header->set = set(key_buffer, value_buffer, (int)incoming_header->k_l, (int)incoming_header->v_l);
                outgoing_header->k_l = outgoing_header->v_l = 0;
                // if this set was called internal, send the information back to the peer
                // who first called for the operation
                if(incoming_header->inl == 1){
                	sendInfosBack(outgoing_header, incoming_header, key_buffer, value_buffer);
                }
                // otherwise call the client directly and close the connection
                else{
                	sendInfosToClient(outgoing_header, incoming_header, key_buffer, value_buffer, temp_socket);
                	close(temp_socket);
                }
            }
            if (incoming_header->get) {
                printf("[recv] Received GET Command\n");
                struct element *e;
                e = get(key_buffer, incoming_header->k_l);
                // if the get operation was successful, set the get bit to 1
                // otherwise to 0 to indicate that the element wasn't found
                if(e != NULL){
                	value_buffer = malloc(e->valuelen);
                    memcpy(value_buffer, e->value, (size_t) e->valuelen);
                    outgoing_header->v_l = e->valuelen;
                    outgoing_header->get = 1;
                    outgoing_header->k_l = e->keylen;
                	printf("FOUND THE ELEMENT IN THE HASHTABLE!\nThis is the Value: %s\n", value_buffer);
                }
                else{
                	printf("DIDN'T FOUND THE ELEMENT IN THE HASHTABLE!\n");
                	value_buffer = NULL;
                	outgoing_header->v_l = 0;
                	outgoing_header->get = 0;
                	outgoing_header->k_l = incoming_header->k_l;
                }
                if(incoming_header->inl == 1){
            		sendInfosBack(outgoing_header, incoming_header, key_buffer, value_buffer);
            	}
            	else{
            		sendInfosToClient(outgoing_header, incoming_header, key_buffer, value_buffer, temp_socket);
            		close(temp_socket);
            	}
            }
            if (incoming_header->del) {
                printf("[recv] Received DEL Command\n");
                outgoing_header->del = del(key_buffer, incoming_header->k_l);
                outgoing_header->k_l = outgoing_header->v_l = 0;
                if(incoming_header->inl == 1){
                	sendInfosBack(outgoing_header, incoming_header, key_buffer, value_buffer);
                }
                else{
                	sendInfosToClient(outgoing_header, incoming_header, key_buffer, value_buffer, temp_socket);
                	close(temp_socket);
                }
            }
        }
        printf("[send] Closed connection\n");

        free(key_buffer);
        free(value_buffer);

        memset(&incoming_header, 0, sizeof incoming_header);
        memset(&outgoing_header, 0, sizeof outgoing_header);
    }

    close(sockfd);
    cleanup();
    return 0;
}
