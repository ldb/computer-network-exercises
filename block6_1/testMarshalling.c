#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "marshalling.h"
 
int main( ) {

	ntp_payload_t *test = (ntp_payload_t*) malloc(sizeof(ntp_payload_t));
	memset(test, 0, sizeof(ntp_payload_t));

	ntp_payload_t *test2 = (ntp_payload_t*) malloc(sizeof(ntp_payload_t));
	memset(test2, 0, sizeof(ntp_payload_t));

	test->li = 1;
	test->vn = 2;
	test->mode = 3;
	test->stratum = 4;
	test->poll = 5;
	test->precision = 6;
	test->root_delay = 7;
	test->root_dispersion = 8;
	test->reference_id = 9;
	test->reference_timestamp = 10;
	test->origin_timestamp = 11;
	test->receive_timestamp = 12;
	test->transmit_timestamp = 13;

	printPayload(test);

	unsigned char testChars[48];
	marshal(&testChars[0], test);

	printBinary(&testChars[0], 12);

	unmarshal(test2, &testChars[0]);

	printPayload(test2);


   return 0;
}