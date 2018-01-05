#ifndef MY_HASHTABLE
#define MY_HASHTABLE

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

unsigned int ht_hash (const char *str, unsigned int length);

struct element *ht_get(char *key, int keylen);

int ht_del(char *key, int keylen);

int ht_set(char *key, char *value, int keylen, int valuelen);

void ht_init(int tablesize);

void ht_cleanup();

#endif
