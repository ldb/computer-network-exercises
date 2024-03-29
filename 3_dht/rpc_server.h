#ifndef RPC_SERVER
#define RPC_SERVER

typedef struct header {
	unsigned int intl : 1;
	unsigned int join : 1;
	unsigned int noti : 1;
	unsigned int stbz : 1;
	unsigned int ack : 1;
	unsigned int get : 1;
	unsigned int set : 1;
	unsigned int del : 1;
	unsigned int tid : 8;
	unsigned short id : 16;
	unsigned int ip : 32;
	unsigned short port: 16;
	unsigned short k_l: 16;
	unsigned short v_l: 16;
} header_t;

int recv_all(header_t *incoming_header, int socket, unsigned char *request_header, char **key_buffer, char **value_buffer, int headersize);

int sendToNextPeer(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer, int temp_socket);

int sendInfosBack(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer);

int sendInfosToClient(header_t *outgoing_header, header_t *incoming_header, char *key_buffer, char *value_buffer, int temp_socket);

header_t *receiveHeader(int socket);

char *receiveKey(int key_len, int socket);

char *receiveValue(int value_len, int socket);

#endif
