#ifndef RPC_SERVER
#define RPC_SERVER

#define CMD_DEL 1
#define CMD_SET 2
#define CMD_GET 4
#define CMD_ACK 8
#define CMD_INL 128

#define HEADER_SIZE_EXT 6
#define HEADER_SIZE_INL 14
#define HASH_SPACE 100
#define SELF_HASH_SPACE 25

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

void printHeader(header_t *header);

void printBinary(char *binaryChar, int len);

int recv_all(header_t *incoming_header, int socket, unsigned char *request_header, char **key_buffer, char **value_buffer, int headersize);

int sendToNextPeer(header_t *outgoing_header, char **key_buffer, char **value_buffer);

#endif
