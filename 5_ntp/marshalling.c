#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "marshalling.h"

void unmarshal(ntp_payload_t *payload, unsigned char *in_payload) {
	payload->li = (unsigned int) (in_payload[0] & LI) >> 6;
	payload->vn = (unsigned int) (in_payload[0] & VN) >> 3;
	payload->mode = (unsigned int) (in_payload[0] & MODE);

	payload->stratum = (unsigned int) in_payload[1];
	payload->poll = (unsigned int) in_payload[2];
	payload->precision = (unsigned int) in_payload[3];

	payload->root_delay = (unsigned int) in_payload[4] << 24;
	payload->root_delay += (unsigned int) (in_payload[5] << 16);
	payload->root_delay += (unsigned int) (in_payload[6] << 8);
	payload->root_delay += (unsigned int) (in_payload[7]);

	payload->root_dispersion = (unsigned int) (in_payload[8] << 24);
	payload->root_dispersion += (unsigned int) (in_payload[9] << 16);
	payload->root_dispersion += (unsigned int) (in_payload[10] << 8);
	payload->root_dispersion += (unsigned int) (in_payload[11]);

	payload->reference_id = (unsigned int) (in_payload[12] << 24);
	payload->reference_id += (unsigned int) (in_payload[13] << 16);
	payload->reference_id += (unsigned int) (in_payload[14] << 8);
	payload->reference_id += (unsigned int) (in_payload[15]);

	payload->reference_timestamp = ((unsigned long)in_payload[16] << 56);
	payload->reference_timestamp +=  ((unsigned long)in_payload[17] << 48);
	payload->reference_timestamp += ((unsigned long)in_payload[18] << 40);
	payload->reference_timestamp += ((unsigned long)in_payload[19] << 32);
	payload->reference_timestamp += ((unsigned long)in_payload[20] << 24);
	payload->reference_timestamp += ((unsigned long)in_payload[21] << 16);
	payload->reference_timestamp += ((unsigned long)in_payload[22] << 8);
	payload->reference_timestamp += ((unsigned long)in_payload[23]);

	payload->origin_timestamp = ((unsigned long)in_payload[24] << 56);
	payload->origin_timestamp += ((unsigned long)in_payload[25] << 48);
	payload->origin_timestamp += ((unsigned long)in_payload[26] << 40);
	payload->origin_timestamp += ((unsigned long)in_payload[27] << 32);
	payload->origin_timestamp += ((unsigned long)in_payload[28] << 24);
	payload->origin_timestamp += ((unsigned long)in_payload[29] << 16);
	payload->origin_timestamp += ((unsigned long)in_payload[30] << 8);
	payload->origin_timestamp += ((unsigned long)in_payload[31]);

	payload->receive_timestamp = ((unsigned long)in_payload[32] << 56);
	payload->receive_timestamp += ((unsigned long)in_payload[33] << 48);
	payload->receive_timestamp += ((unsigned long)in_payload[34] << 40);
	payload->receive_timestamp += ((unsigned long)in_payload[35] << 32);
	payload->receive_timestamp += ((unsigned long)in_payload[36] << 24);
	payload->receive_timestamp += ((unsigned long)in_payload[37] << 16);
	payload->receive_timestamp += ((unsigned long)in_payload[38] << 8);
	payload->receive_timestamp += ((unsigned long)in_payload[39]);

	payload->transmit_timestamp = ((unsigned long)in_payload[40] << 56);
	payload->transmit_timestamp += ((unsigned long)in_payload[41] << 48);
	payload->transmit_timestamp += ((unsigned long)in_payload[42] << 40);
	payload->transmit_timestamp += ((unsigned long)in_payload[43] << 32);
	payload->transmit_timestamp += ((unsigned long)in_payload[44] << 24);
	payload->transmit_timestamp += ((unsigned long)in_payload[45] << 16);
	payload->transmit_timestamp += ((unsigned long)in_payload[46] << 8);
	payload->transmit_timestamp += ((unsigned long)in_payload[47]);
}

void marshal(unsigned char *out_payload, ntp_payload_t *payload) {
	out_payload[0] = (unsigned char) (out_payload[0] & ~LI) | ((payload->li << 6) & LI);
	out_payload[0] = (unsigned char) (out_payload[0] & ~VN) | ((payload->vn << 3) & VN) ;
	out_payload[0] = (unsigned char) (out_payload[0] & ~MODE) | ((payload->mode) & MODE);

	out_payload[1] = (unsigned char) payload->stratum;
	out_payload[2] = (unsigned char) payload->poll;
	out_payload[3] = (unsigned char) payload->precision;

	out_payload[4] = (unsigned char) (payload->root_delay >> 24);
	out_payload[5] = (unsigned char) (payload->root_delay >> 16);
	out_payload[6] = (unsigned char) (payload->root_delay >> 8);
	out_payload[7] = (unsigned char) (payload->root_delay % 256);

	out_payload[8] = (unsigned char) (payload->root_dispersion >> 24);
	out_payload[9] = (unsigned char) (payload->root_dispersion >> 16);
	out_payload[10] = (unsigned char) (payload->root_dispersion >> 8);
	out_payload[11] = (unsigned char) (payload->root_dispersion % 256);

	out_payload[12] = (unsigned char) (payload->reference_id >> 24);
	out_payload[13] = (unsigned char) (payload->reference_id >> 16);
	out_payload[14] = (unsigned char) (payload->reference_id >> 8);
	out_payload[15] = (unsigned char) (payload->reference_id % 256);

	out_payload[16] = (unsigned char) (payload->reference_timestamp >> 56);
	out_payload[17] = (unsigned char) (payload->reference_timestamp >> 48);
	out_payload[18] = (unsigned char) (payload->reference_timestamp >> 40);
	out_payload[19] = (unsigned char) (payload->reference_timestamp >> 32);
	out_payload[20] = (unsigned char) (payload->reference_timestamp >> 24);
	out_payload[21] = (unsigned char) (payload->reference_timestamp >> 16);
	out_payload[22] = (unsigned char) (payload->reference_timestamp >> 8);
	out_payload[23] = (unsigned char) (payload->reference_timestamp % 256);

	out_payload[24] = (unsigned char) (payload->origin_timestamp >> 56);
	out_payload[25] = (unsigned char) (payload->origin_timestamp >> 48);
	out_payload[26] = (unsigned char) (payload->origin_timestamp >> 40);
	out_payload[27] = (unsigned char) (payload->origin_timestamp >> 32);
	out_payload[28] = (unsigned char) (payload->origin_timestamp >> 24);
	out_payload[29] = (unsigned char) (payload->origin_timestamp >> 16);
	out_payload[30] = (unsigned char) (payload->origin_timestamp >> 8);
	out_payload[31] = (unsigned char) (payload->origin_timestamp % 256);

	out_payload[32] = (unsigned char) (payload->receive_timestamp >> 56);
	out_payload[33] = (unsigned char) (payload->receive_timestamp >> 48);
	out_payload[34] = (unsigned char) (payload->receive_timestamp >> 40);
	out_payload[35] = (unsigned char) (payload->receive_timestamp >> 32);
	out_payload[36] = (unsigned char) (payload->receive_timestamp >> 24);
	out_payload[37] = (unsigned char) (payload->receive_timestamp >> 16);
	out_payload[38] = (unsigned char) (payload->receive_timestamp >> 8);
	out_payload[39] = (unsigned char) (payload->receive_timestamp % 256);

	out_payload[40] = (unsigned char) (payload->transmit_timestamp >> 56);
	out_payload[41] = (unsigned char) (payload->transmit_timestamp >> 48);
	out_payload[42] = (unsigned char) (payload->transmit_timestamp >> 40);
	out_payload[43] = (unsigned char) (payload->transmit_timestamp >> 32);
	out_payload[44] = (unsigned char) (payload->transmit_timestamp >> 24);
	out_payload[45] = (unsigned char) (payload->transmit_timestamp >> 16);
	out_payload[46] = (unsigned char) (payload->transmit_timestamp >> 8);
	out_payload[47] = (unsigned char) (payload->transmit_timestamp % 256);
}

void printPayload(ntp_payload_t *payload) {
	printf("li        : %d\n", payload->li);
	printf("vn        : %d\n", payload->vn);
	printf("mode      : %d\n", payload->mode);
	printf("stratum   : %d\n", payload->stratum);
	printf("poll      : %d\n", payload->poll);
	printf("prec      : %d\n", payload->precision);
	printf("root del  : %d\n", payload->root_delay);
	printf("root disp : %d\n", payload->root_dispersion);
	printf("ref id    : %d\n", payload->reference_id);
	printf("ref ts    : %lu\n", payload->reference_timestamp);
	printf("origin ts : %lu\n", payload->origin_timestamp);
	printf("recv ts   : %lu\n", payload->receive_timestamp);
	printf("transm ts : %lu\n", payload->transmit_timestamp);
}

void printBinary(unsigned char *binaryChar, int len) {
	for (int j = 0; j < len * 4; j++) {
		if ((j > 0) && (j % 4 == 0)) {
			printf("-----------\n");
		}
		for (int i = 0; i < 8; i++) {
			printf("%d", !!((binaryChar[j] << i) & 0x80));
		}
		printf(" %d \n", binaryChar[j]);
	}
}
