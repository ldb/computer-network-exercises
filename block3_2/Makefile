BIN = client
ABIN = hashtable.c
HOST = localhost
PORT = 8080
FILE = filme.csv

SRC = \
	client.c	

ARCH = $(shell getconf LONG_BIT)


CC	 = gcc
CFLAGS   = -g -Wall $(SDL_CFLAGS_$(ARCH))
LFLAGS   += $(SDL_LFLAGS_$(ARCH))


GDB    = gdb
RM     = rm -f

OBJS = $(SRC:%.c=%.o)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(ABIN) $(OBJS) $(LFLAGS)

# pull in dependencies for .o files
-include $(OBJS:.o=.d)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
	$(CC) -MM $(CFLAGS) $*.c > $*.d


debug: $(BIN)
	$(GDB) ./$(BIN)

run: $(BIN)
	./$(BIN) $(HOST) $(PORT) $(FILE)

clean:
	$(RM) $(BIN) $(OBJS) *.d $(OBIN)

.PHONY: clean debug run 
