#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "marshalling.h"
#include "rpc_server.h"

void unmarshal(header_t *out_header, unsigned char *in_header) {
	out_header->intl = (unsigned int) (in_header[0] & CMD_INTL) == CMD_INTL;
	out_header->join = (unsigned int) (in_header[0] & CMD_JOIN) == CMD_JOIN;
	out_header->noti = (unsigned int) (in_header[0] & CMD_NOTI) == CMD_NOTI;
	out_header->stbz = (unsigned int) (in_header[0] & CMD_STBZ) == CMD_STBZ;

	out_header->ack = (unsigned int) (in_header[0] & CMD_ACK) == CMD_ACK;
	out_header->get = (unsigned int) (in_header[0] & CMD_GET) == CMD_GET;
	out_header->set = (unsigned int) (in_header[0] & CMD_SET) == CMD_SET;
	out_header->del = (unsigned int) (in_header[0] & CMD_DEL) == CMD_DEL;
	
	out_header->tid = (unsigned int) in_header[1];
	out_header->k_l = (unsigned short) (in_header[2] << 8);
	out_header->k_l += (unsigned short) (in_header[3]);
	out_header->v_l = (unsigned short) (in_header[4] << 8);
	out_header->v_l += (unsigned short) (in_header[5]);

	if (!out_header->intl) {
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
	out_header[0] += (unsigned char) in_header->get * CMD_GET;
	out_header[0] += (unsigned char) in_header->ack * CMD_ACK;
	out_header[0] += (unsigned char) (in_header->set * CMD_SET);
	out_header[0] += (unsigned char) (in_header->del * CMD_DEL);
	
	out_header[1] = (unsigned char) in_header->tid;
	out_header[2] = (unsigned char) (in_header->k_l >> 8);
	out_header[3] = (unsigned char) (in_header->k_l % 256);
	out_header[4] = (unsigned char) (in_header->v_l >> 8);
	out_header[5] = (unsigned char) (in_header->v_l % 256);

	if (!in_header->intl) {
		return;
	}

	out_header[0] += (unsigned char) (in_header->intl * CMD_INTL);
	out_header[0] += (unsigned char) in_header->join * CMD_JOIN;
	out_header[0] += (unsigned char) in_header->noti * CMD_NOTI;
	out_header[0] += (unsigned char) in_header->stbz * CMD_STBZ;

	out_header[6] = (unsigned char) (in_header->id >> 8);
	out_header[7] = (unsigned char) (in_header->id % 256);
	out_header[8] = (unsigned char) (in_header->ip >> 24);
	out_header[9] = (unsigned char) (in_header->ip >> 16);
	out_header[10] = (unsigned char) (in_header->ip >> 8);
	out_header[11] = (unsigned char) (in_header->ip % 256);
	out_header[12] = (unsigned char) (in_header->port >> 8);
	out_header[13] = (unsigned char) (in_header->port % 256);
}

void printHeader(header_t *header) {
	printf("ack : %d\n", header->ack);
	printf("get : %d\n", header->get);
	printf("set : %d\n", header->set);
	printf("del : %d\n", header->del);
	printf("tid : %d\n", header->tid);
	printf("k_l : %d\n", header->k_l);
	printf("v_l : %d\n", header->v_l);

	if (!header->intl) {
		return;
	}

	printf("intl: %d\n", header->intl);
	printf("join: %d\n", header->join);
	printf("noti: %d\n", header->noti);
	printf("stbz: %d\n", header->stbz);
}

void printBinary(char *binaryChar, int len) {
	for (int j = 0; j < len; j++) {
		for (int i = 0; i < 8; i++) {
			printf("%d", !!((binaryChar[j] << i) & 0x80));
		}
		printf(" %c\n", binaryChar[j]);
	}
}
