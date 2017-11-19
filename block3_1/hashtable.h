#ifndef HASHTABLE
#define HASHTABLE

struct element {
	char *key;
	char *value;
	int keylen;
	int valuelen;
	struct element *next;
	struct element *previous;
};

struct hash {
	struct element *head;
	int count;
};

unsigned int hash (const char *str, unsigned int length);

struct element *createElement(char *key, char *value, int keylen, int valuelen);

struct element *get(char *key, int keylen);

int del(char *key, int keylen);

void setElement(struct element *e);

int set(char *key, char *value, int keylen, int valuelen);

void init(int tablesize);

void cleanup();

#endif
