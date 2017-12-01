#ifndef MARSHALLING
#define MARSHALLING

#include "rpc_server.h"

void unmarshal(header_t *out_header, unsigned char *in_header);

void marshal(char *out_header, header_t *in_header);

#endif