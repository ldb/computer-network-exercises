#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "marshalling.h"
#include "rpc_server.h"

void unmarshal(header_t *out_header, unsigned char *in_header) {
    out_header->set = (unsigned int) (in_header[0] & CMD_SET) == CMD_SET;
    out_header->del = (unsigned int) (in_header[0] & CMD_DEL) == CMD_DEL;
    out_header->get = (unsigned int) (in_header[0] & CMD_GET) == CMD_GET;
    out_header->ack = (unsigned int) (in_header[0] & CMD_ACK) == CMD_ACK;
    out_header->inl = (unsigned int) (in_header[0] & CMD_INL) == CMD_INL;
    out_header->tid = (unsigned int) in_header[1];
    out_header->k_l = (unsigned short) (in_header[2] << 8);
    out_header->k_l += (unsigned short) (in_header[3]);
    out_header->v_l = (unsigned short) (in_header[4] << 8);
    out_header->v_l += (unsigned short) (in_header[5]);

    if (!out_header->inl || out_header->inl == 0) {
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
    out_header[0] += (unsigned char) (in_header->set * CMD_SET);
    out_header[0] += (unsigned char) (in_header->del * CMD_DEL);
    out_header[0] += (unsigned char) in_header->get * CMD_GET;
    out_header[0] += (unsigned char) in_header->ack * CMD_ACK;
    out_header[1] = (unsigned char) in_header->tid;
    out_header[2] = (unsigned char) (in_header->k_l >> 8);
    out_header[3] = (unsigned char) (in_header->k_l % 256);
    out_header[4] = (unsigned char) (in_header->v_l >> 8);
    out_header[5] = (unsigned char) (in_header->v_l % 256);

    if (!in_header->inl || in_header->inl == 0) {
        return;
    }

    out_header[0] += (unsigned char) (in_header->inl * CMD_INL);
    out_header[6] = (unsigned char) (in_header->id >> 8);   //die ersten 8 Stellen der ID als MSB, erste Hälfte ins erste Byte der ID
    out_header[7] = (unsigned char) (in_header->id % 256);  //die letzten 8 Stellen der ID als LSB, zweite Hälfte ins zweite Byte der ID
    out_header[8] = (unsigned char) (in_header->ip >> 24);
    out_header[9] = (unsigned char) (in_header->ip >> 16);
    out_header[10] = (unsigned char) (in_header->ip >> 8);
    out_header[11] = (unsigned char) (in_header->ip % 256);
    out_header[12] = (unsigned char) (in_header->port >> 8);
    out_header[13] = (unsigned char) (in_header->port % 256);
}