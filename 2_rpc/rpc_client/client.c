#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include "rpc_interface.h"

#define COUNT 25

int arrayContains(int *rnmbrs, int length, int number) {
	for (int i = 0; i < length; i++) {
		if (rnmbrs[i] == number) {
			return 1;
		}
	}

	return 0;
}

// Get COUNT random lines from a given file.
// The key is the first string until the first ; occurs.
void getFileInfo(char *file, char **data, char **keys, char **values) {
	FILE *fp = fopen(file, "r");
	int lines = 1;
	int rnmbrs[COUNT];
	int number = 0;
	srand(time(NULL));
	char ch;

	while (!feof(fp)) {
		ch = fgetc(fp);
		if (ch == '\n') {
			lines++;
		}
	}

	int i = 0;
	while (i < COUNT) {
		number = rand() % lines;
		if (arrayContains(rnmbrs, lines, number) == 0) {
			rnmbrs[i] = number;
			i++;
		}
	}

	// Get the data from each line and store them in keys/values.
	rewind(fp);
	ch = ' ';

	for (int i = 0; i < COUNT; i++) {
		int j = 1;

		while (j < rnmbrs[i]) {
			while ((ch = fgetc(fp)) != '\n') {
			}
			j++;
		}
		fgets(data[i], 512, fp);
		strcpy(keys[i], strtok(data[i], ";"));
		strcpy(values[i], strtok(NULL, "\n"));
		rewind(fp);
	}

	fclose(fp);

	return;
}

// Perform COUNT sets, gets and deletes and then try to get these elements again.
// (Has to get the Host and Port when communicating with the server)
void setGetDel(char **keys, char**values, char *dns, char *port) {
	for (int i = 0; i < COUNT; i++) {
		int res = set(keys[i], values[i], strlen(keys[i]), strlen(values[i]));
		printf("[c] send: %d response: %d\n", i + 1, res);
	}

	struct element *e;

	for (int i = 0; i < COUNT; i++) {
		e = get(keys[i], strlen(keys[i]));

		if (e != NULL) {
			char *newKey = malloc(e->keylen + 1);
			memcpy(newKey, e->key, e->keylen);
			newKey[e->keylen] = '\0';
			char *newValue = malloc(e->valuelen + 1);
			memcpy(newValue, e->value, e->valuelen);
			newValue[e->valuelen] = '\0';
			printf("[c] [%d]key: %s\n    [%d]value: %s\n", i, newKey, i, newValue);
			free(e->key);
			free(e->value);
			free(e);
			free(newKey);
			free(newValue);
		} else {
			fprintf(stderr, "[c] Could not find element with key: %s\n", keys[i]);
		}
	}

	for (int i = 0; i < COUNT; i++) {
		del(keys[i], strlen(keys[i]));
	}

	for (int i = 0; i < COUNT; i++) {
		e = get(keys[i], strlen(keys[i]));

		if (e != NULL) {
			char *newKey = malloc(e->keylen + 1);
			memcpy(newKey, e->key, e->keylen);
			newKey[e->keylen] = '\0';
			char *newValue = malloc(e->valuelen + 1);
			memcpy(newValue, e->value, e->valuelen);
			newValue[e->valuelen] = '\0';
			printf("[c] [%d]keys: %s\n    [%d]value: %s\n", i, newKey, i, newValue);
			free(e->key);
			free(e->value);
			free(e);
			free(newKey);
			free(newValue);
		} else {
			fprintf(stderr, "[c] Could not find element with key: %s\n", keys[i]);
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		fprintf(stderr, "arguments: HOST PORT FILE");
		return 0;
	}

	char **data = malloc(sizeof(char*)*COUNT);
	char **keys = malloc(sizeof(char*)*COUNT);
	char **values = malloc(sizeof(char*)*COUNT);

	for (int i = 0; i < COUNT; i++) {
		data[i] = malloc(sizeof(char) * 1024);
		keys[i] = malloc(sizeof(char) * 128);
		values[i] = malloc(sizeof(char) * 996);
	}

	getFileInfo(argv[3], data, keys, values);

	init(argv[1], argv[2]);

	setGetDel(keys, values, argv[1], argv[2]);

	for (int i = 0; i < COUNT; i++) {
		free(data[i]);
		free(keys[i]);
		free(values[i]);
	}

	free(data);
	free(keys);
	free(values);
	return 0;
}
