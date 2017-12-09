#ifndef MARSHALLING
#define MARSHALLING

#include "rpc_server.h"

#define CMD_DEL 1
#define CMD_SET 2
#define CMD_GET 4
#define CMD_ACK 8
#define CMD_STBZ 16
#define CMD_NOTI 32
#define CMD_JOIN 64
#define CMD_INTL 128

void unmarshal(header_t *out_header, unsigned char *in_header);

void marshal(char *out_header, header_t *in_header);

#endif