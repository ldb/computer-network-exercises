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

#define CMD_DEL 1
#define CMD_SET 2
#define CMD_GET 4
#define CMD_ACK 8

int HEADER_SIZE = 6;
char *HOST;
char *PORT;

typedef struct header {
	unsigned int set : 1;
	unsigned int get : 1;
	unsigned int del : 1;
	unsigned int ack : 1;
	unsigned int tid : 8;
	unsigned short key_length;
	unsigned short value_length;
} header_t;

typedef struct element {
	char *key;
	char *value;
	int keylen;
	int valuelen;
} element_t;

void unmarshal(header_t *out_header, char *in_header) {
	out_header->set = (int)(in_header[0] & CMD_SET) == CMD_SET;
	out_header->del = (int)(in_header[0] & CMD_DEL) == CMD_DEL;
	out_header->get = (int)(in_header[0] & CMD_GET) == CMD_GET;
	out_header->ack = (int)(in_header[0] & CMD_ACK) == CMD_ACK;
	out_header->tid = (int)in_header[1];
	out_header->key_length = (short)(in_header[2] << 8);
	out_header->key_length += (short)(in_header[3]);
	out_header->value_length = (short)(in_header[4] << 8);
	out_header->value_length += (short)(in_header[5]);
}

void marshal(char *out_header, header_t *in_header) {
	out_header[0] += (unsigned char)in_header->set * CMD_SET;
	out_header[0] += (unsigned char)in_header->del * CMD_DEL;
	out_header[0] += (unsigned char)in_header->get * CMD_GET;
	out_header[0] += (unsigned char)in_header->ack * CMD_ACK;
	out_header[1] = (unsigned char)in_header->tid;
	out_header[2] = (unsigned char)(in_header->key_length >> 8);
	out_header[3] = (unsigned char)(in_header->key_length % 256);
	out_header[4] = (unsigned char)(in_header->value_length >> 8);
	out_header[5] = (unsigned char)(in_header->value_length % 256);
}

void printHeader(header_t *header) {
	printf("set: %d\n", header->set);
	printf("get: %d\n", header->get);
	printf("del: %d\n", header->del);
	printf("ack: %d\n", header->ack);
	printf("tid: %d\n", header->tid);
	printf("k_l: %d\n", header->key_length);
	printf("v_l: %d\n", header->value_length);
}

void printBinary(char *binaryChar, int len) {
	for (int j = 0; j < len; j++) {
		for (int i = 0; i < 8; i++) {
			printf("%d", !!((binaryChar[j] << i) & 0x80));
		}
		printf(" %c\n", binaryChar[j]);
	}
}

int do_rpc(char *out_buffer, int size, element_t *element) {
	struct addrinfo hints, *res;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(HOST, PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", strerror(status));
		return 2;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	
	if(connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
		fprintf(stderr, "connect: %s\n", strerror(errno));
		return 2;
	}

	int to_send = size;
	do {
		int sent;
		if((sent = send(sockfd, out_buffer, to_send, 0)) == -1){
			fprintf(stderr, "send: %s\n", strerror(errno));
			return 2;
		}

		printf("sent: %d\n", sent);

		to_send -= sent;
		out_buffer += sent;
	} while (0 < to_send);
	out_buffer -= size;

	printf("done sending:\n");
	
	char request_header[HEADER_SIZE];
	char *request_ptr = request_header;
	memset(&request_header, 0, HEADER_SIZE);

	int read_size = sizeof request_header;

	ssize_t rs = 0;
	ssize_t read = 0;
	int twice = 0;

	header_t incoming_header;
	char *key_buffer = NULL;
	char *value_buffer = NULL;
	printf("recving:\n");
	do {
		if ((rs = recv(sockfd, request_ptr, read_size, 0)) < 0) {
			fprintf(stderr, "recv: %s\n", strerror(errno));
			return 2;
		}
		read += rs;

		printf("recv:\n");

		if (twice == 0) {
			twice++;
			unmarshal(&incoming_header, request_ptr);

			printHeader(&incoming_header);

			read_size = incoming_header.key_length;
			request_ptr = key_buffer = malloc(read_size);

			read = 0;
			continue;
		}

		if (read == incoming_header.key_length && incoming_header.value_length > 0 && twice == 1) {
			twice++;
			request_ptr = value_buffer = malloc(incoming_header.value_length);
			read_size = incoming_header.value_length;
			read = 0;
			continue;
		}

		request_ptr += rs;
	} while (rs > 0 && read < read_size);

	printf("done recving:\n");

	close(sockfd);

	if (incoming_header.get == 1) {
		printBinary(key_buffer, incoming_header.key_length);
		printBinary(value_buffer, incoming_header.value_length);
		element->key = key_buffer;
		element->value = value_buffer;
		element->keylen = incoming_header.key_length;
		element->valuelen = incoming_header.value_length;
	}
	
	return 0;
}

int set(char *key, char *value, int keylen, int valuelen) {
	header_t outgoing_header;
	memset(&outgoing_header, 0, sizeof outgoing_header);
	outgoing_header.set = 1;
	outgoing_header.key_length = keylen;
	outgoing_header.value_length = valuelen;

	char h[6] = "00000";
	char *out_header = h;
	marshal(out_header, &outgoing_header);

	size_t final_size = outgoing_header.key_length + outgoing_header.value_length + HEADER_SIZE;
	char *outbuffer = malloc(final_size);
	memcpy(outbuffer, out_header, HEADER_SIZE);
	memcpy(outbuffer + HEADER_SIZE, key, outgoing_header.key_length);
	memcpy(outbuffer + HEADER_SIZE + outgoing_header.key_length, value, outgoing_header.value_length);

	printBinary(outbuffer, final_size);

	element_t *e;
	int res = do_rpc(outbuffer, final_size, e);

	printf("returned: %d\n", res);
	free(outbuffer);
	return 0;
}

int del(char *key, int keylen) {
	header_t outgoing_header;
	memset(&outgoing_header, 0, sizeof outgoing_header);
	outgoing_header.del = 1;
	outgoing_header.key_length = keylen;

	char h[6] = "000000";
	char *out_header = h;

	marshal(out_header, &outgoing_header);
	size_t final_size = outgoing_header.key_length + HEADER_SIZE;

	char *outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE);
	memcpy(outbuffer + HEADER_SIZE, key, outgoing_header.key_length);
	
	printBinary(outbuffer, final_size);

	element_t *e;
	int res = do_rpc(outbuffer, final_size, e);

	free(outbuffer);
	return 0;
}

struct element *get(char *key, int keylen) {
	header_t outgoing_header;
	memset(&outgoing_header, 0, sizeof outgoing_header);
	outgoing_header.get = 1;
	outgoing_header.key_length = keylen;

	char h[6] = "000000";
	char *out_header = h;
	marshal(out_header, &outgoing_header);
	size_t final_size = outgoing_header.key_length + HEADER_SIZE;
	char *outbuffer = malloc(final_size);

	memcpy(outbuffer, out_header, HEADER_SIZE);
	memcpy(outbuffer + HEADER_SIZE, key, outgoing_header.key_length);

	printBinary(outbuffer, final_size);

	element_t *e = malloc(sizeof(element_t));

	int res = do_rpc(outbuffer, final_size, e);

	free(outbuffer);
	return e;
}

void init(char *host, char *port) {
	HOST = host;
	PORT = port;
}

int main(int argc, char *argv[]) {

	init("localhost", "1717");

	char *testkey1 = "key1";
	char *testval1 = "val1";

	set(testkey1, testval1, 4, 4);

	char *testkey2 = "key1";
	char *testval2 = "val2";

	element_t *e = get(testkey1, 4);
	printf("%s, %s\n",e->key, e->value);

	char *testkey3 = "key3";
	char *testval3 = "val3";

	//del(testkey1, 4);

	char *testkey4 = "key4";
	char *testval4 = "val4";
}