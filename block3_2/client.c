#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>


int main(int argc, char *argv[]){
	if(argc != 4){
		fprintf(stderr, "Use: ./client HOST PORT FILE");
		return 0;
	}
}