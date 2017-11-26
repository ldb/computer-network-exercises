#ifndef RPC_INTERFACE_H
#define RPC_INTERFACE_H

struct element {
	char *key;
	char *value;
	int keylen;
	int valuelen;
};

struct element *get(char *key, int keylen);

int del(char *key, int keylen);

int set(char *key, char *value, int keylen, int valuelen);

void init(char *host, char *port);

#endif
