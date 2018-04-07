64:
	gcc -o server -I include -L lib/64 include/hashtable/hashtable.c server.c -pthread -lonion_static -lrt

32:
	gcc -o server -I include -L lib/32 include/hashtable/hashtable.c server.c -pthread -lonion_static -lrt

all:
	gcc -o server -I include -L lib/32 -L lib/64 include/hashtable/hashtable.c server.c -pthread -lonion_static -lrt
