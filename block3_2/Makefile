BIN = clients
SBIN = servers
ABIN = rpc_interface.c
BBIN = hashtable.c
HOST = localhost
PORT = 8080
FILE = filme.csv

SRC = \
	client.c
SRC2 = \
	rpc_server.c

ARCH = $(shell getconf LONG_BIT)


CC	 = gcc
CFLAGS   = -g $(SDL_CFLAGS_$(ARCH))
LFLAGS   += $(SDL_LFLAGS_$(ARCH))


GDB    = gdb
RM     = rm -f

OBJC = $(SRC:%.c=%.o)
OBJS = $(SRC2:%.c=%.o)

$(BIN): $(OBJC)
	$(CC) $(CFLAGS) -o $(BIN) $(ABIN) $(OBJC) $(LFLAGS)

$(SBIN): $(OBJS)
	$(CC) $(CFLAGS) -o $(SBIN) $(BBIN) $(OBJS) $(LFLAGS)

# pull in dependencies for .o files	
-include $(OBJC:.o=.d)
-include $(OBJS:.o=.d)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
	$(CC) -MM $(CFLAGS) $*.c > $*.d

client: $(BIN)
	./$(BIN) $(HOST) $(PORT) $(FILE)

server: $(SBIN)
	./$(SBIN) $(PORT)

clean:
	$(RM) $(SBIN) $(BIN) $(OBJS) $(OBJC) *.d $(OBIN)

.PHONY: clean debug run 
