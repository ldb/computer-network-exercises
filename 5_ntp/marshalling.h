#ifndef MARSHALLING
#define MARSHALLING

#define MODE 7
#define VN 56
#define LI 192

typedef struct payload {
	unsigned int li : 2;
	unsigned int vn : 3;
	unsigned int mode : 3;
	unsigned int stratum : 8;
	unsigned int poll : 8;
	unsigned int precision : 8;
	unsigned int root_delay: 32;
	unsigned int root_dispersion: 32;
	unsigned int reference_id : 32;
	unsigned long reference_timestamp : 64;
	unsigned long origin_timestamp: 64;
	unsigned long receive_timestamp: 64;
	unsigned long transmit_timestamp: 64;
} ntp_payload_t;

void printPayload(ntp_payload_t *payload);

void printBinary(unsigned char *binaryChar, int len);

void unmarshal(ntp_payload_t *payload, unsigned char *in_payload);

void marshal(unsigned char *out_payload, ntp_payload_t *payload);

#endif