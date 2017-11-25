#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include "rpc_interface.h"

#define COUNT 3


// check if a given array contains the searched value
int arrayContains(int *rnmbrs, int length, int number){
	for(int i = 0; i < length; i++){
		if(rnmbrs[i] == number){
			return 1;
		}
	}

	return 0;
}

// get 25 random lines from a given file
// the key is the first string until the first ; occurs
// the information is stored combined
void getFileInfo(char *file, char **data, char **keys, char **values){
	FILE *fp = fopen(file, "r");
	int lines = 1;
	int rnmbrs[COUNT];
	int number = 0;
	srand(time(NULL));
	char ch;

	// count the lines in the file
	// file has to have an empty newline at the end
	while(!feof(fp)){
	  	ch = fgetc(fp);
	  	if(ch == '\n')
	  	{
	  		lines++;
	  	}
	}

	// create 25 random and different numbers
	// in range of the lines
	int i = 0;
	while(i < COUNT){
		number = rand() % lines;
		if(arrayContains(rnmbrs, lines, number) == 0){
			rnmbrs[i] = number;
			i++;
		}

	}

	// get the data from each line and
	// store them in keys/values
	rewind(fp);
	ch = ' ';

	for(int i = 0; i < COUNT; i++){
		int j = 1;

		while(j < rnmbrs[i]){
			while((ch=fgetc(fp)) != '\n'){
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


// Perform 25 sets, gets and deletes and then try to get these elements again
// (Has to get the Host and Port when communicating with the server)
void setGetDel(char **keys, char**values, char *dns, char *port){
	for(int i = 0; i < COUNT; i++){
		int res = set(keys[i], values[i], strlen(keys[i]), strlen(values[i]));
		printf("send: %d\nres: %d\n", i+1, res);
	}

	struct element *e;

	for(int i = 0; i < COUNT; i++){
		e = get(keys[i], strlen(keys[i]));

		if(e != NULL){
			char *newKey = malloc(e->keylen+1);
			memcpy(newKey, e->key, e->keylen);
			newKey[e->keylen] = '\0';
			char *newValue = malloc(e->valuelen+1);
			memcpy(newValue, e->value, e->valuelen);
			newValue[e->valuelen] = '\0';
			printf("keys[%d]: %s\nvalues[%d]: %s\n", i, newKey, i, newValue);
		}
		else{
			fprintf(stderr, "Couldn't find the element with the key: %s\n", keys[i]);
		}
	}

	for(int i = 0; i < COUNT; i++){
		del(keys[i], strlen(keys[i]));
	}

	for(int i = 0; i < COUNT; i++){
		e = get(keys[i], strlen(keys[i]));

		if(e != NULL){
			char *newKey = malloc(e->keylen+1);
			memcpy(newKey, e->key, e->keylen);
			newKey[e->keylen] = '\0';
			char *newValue = malloc(e->valuelen+1);
			memcpy(newValue, e->value, e->valuelen);
			newValue[e->valuelen] = '\0';
			printf("keys[%d]: %s\nvalues[%d]: %s\n", i, newKey, i, newValue);
		}
		else{
			fprintf(stderr, "Couldn't find the element with the key: %s\n", keys[i]);
		}
	}
}

int main(int argc, char *argv[]){
	if(argc != 4){
		fprintf(stderr, "Use: ./client HOST PORT FILE");
		return 0;
	}

	char **data = malloc(sizeof(char*)*COUNT);			// saves all the readed data
	char **keys = malloc(sizeof(char*)*COUNT);			// saves all keys
	char **values = malloc(sizeof(char*)*COUNT);		// saves all values

	for(int i = 0; i < COUNT; i++){
		data[i] = malloc(sizeof(char)*1024);		// saves a line from the file
		keys[i] = malloc(sizeof(char)*128);			// saves the title of a movie
		values[i] = malloc(sizeof(char)*996);		// saves more information about the movie
	}

	getFileInfo(argv[3], data, keys, values);

	init(argv[1], argv[2]);		// Initialize Socket in the rpc_interface

	setGetDel(keys, values, argv[1], argv[2]);

	for(int i = 0; i < COUNT; i++){	// Free all the allocated space
		free(data[i]);
		free(keys[i]);
		free(values[i]);
	}

	free(data);			// Free all double pointer
	free(keys);
	free(values);

	return 0;
}