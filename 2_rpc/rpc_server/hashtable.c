#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include "hashtable.h"

struct hash *hashtable = NULL;
int tl;

// the DJB2 hash function
unsigned int hash(const char *str, unsigned int length) {
	unsigned int hash = 5381;
	unsigned int i = 0;

	for (i = 0; i < length; ++str, ++i) {
		hash = ((hash << 5) + hash) + (*str);
	}

	return hash;
}

struct element *createElement(char *key, char *value, int keylen, int valuelen) {
	struct element *newElem;

	newElem = (struct element*) malloc(sizeof(struct element));

	newElem->key = (char*)malloc(keylen);
	newElem->value = (char*)malloc(valuelen);

	memcpy(newElem->key, key, keylen);
	memcpy(newElem->value, value, valuelen);
	newElem->keylen = keylen;
	newElem->valuelen = valuelen;
	newElem->next = NULL;
	newElem->previous = NULL;

	return newElem;
}

struct element *get(char *key, int keylen) {
	unsigned int hashValue = hash(key, keylen) % tl;
	struct element *searchElement;
	searchElement = hashtable[hashValue].head;

	if (searchElement == NULL) {
		return NULL;
	}

	while (searchElement != NULL) {
		if (memcmp(searchElement->key, key, keylen) == 0) {
			return searchElement;
		}
		searchElement = searchElement->next;
	}

	return NULL;
}

int del(char *key, int keylen) {
	struct element *e;
	unsigned int hashValue = hash(key, keylen) % tl;

	if ((e = get(key, keylen)) == NULL) {
		fprintf(stderr, "Unable to find the element!\n");
		return 0;
	}

	if (hashtable[hashValue].head == e) {
		hashtable[hashValue].head = e->next;
	}
	if (e->previous != NULL) {
		e->previous->next = e->next;
	}
	if (e->next != NULL) {
		e->next->previous = e->previous;
	}

	hashtable[hashValue].count--;

	free(e->key);
	free(e->value);

	free(e);

	return 1;
}

void setElement(struct element *e) {
	unsigned int hashValue = hash(e->key, e->keylen) % tl;

	struct element *tmp = hashtable[hashValue].head;

	if (tmp == NULL) {
		hashtable[hashValue].head = e;
		hashtable[hashValue].count = 1;
		return;
	}

	while (tmp != NULL) {
		if (memcmp(tmp->key, e->key, e->keylen) == 0) {
			del(tmp->key, tmp->keylen);
			setElement(e);
			return;
		}
		tmp = tmp->next;
	}

	e->next = hashtable[hashValue].head;
	hashtable[hashValue].head->previous = e;
	hashtable[hashValue].head = e;
	hashtable[hashValue].count++;
}

int set(char *key, char *value, int keylen, int valuelen) {
	struct element *e = createElement(key, value, keylen, valuelen);
	setElement(e);

	return 1;
}

void init(int tablesize) {
	tl = tablesize;
	hashtable = (struct hash*)calloc(tl, sizeof(struct hash));
}

void cleanup() {
	free(hashtable);
}
