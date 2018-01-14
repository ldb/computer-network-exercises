#include <stdio.h>
#include <string.h>
#include <time.h>

int main( ) {

	clock_t start = clock();

	int x;
	for(unsigned int i = 0; i<900000000;i++) {
		x++;
	}
	
	printf("%lu\n", (clock()-start));
	return 0;
}
