BIN = rpc_server
ABIN = marshalling.c
BBIN = hashtable.c

SRC = \
	rpc_server.c

ARCH = $(shell getconf LONG_BIT)

CC = gcc
CFLAGS = -g $(SDL_CFLAGS_$(ARCH))
LFLAGS += $(SDL_LFLAGS_$(ARCH))

RM     = rm -rf

OBJC = $(SRC:%.c=%.o)
OBJS = $(SRC2:%.c=%.o)

$(BIN): $(OBJC)
	$(CC) $(CFLAGS) -o $(BIN) $(ABIN) $(BBIN) $(OBJC) $(LFLAGS)

# pull in dependencies for .o files	
-include $(OBJC:.o=.d)
-include $(OBJS:.o=.d)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
	$(CC) -MM $(CFLAGS) $*.c > $*.d

clean:
	$(RM) $(SBIN) $(BIN) $(OBJS) $(OBJC) *.d $(OBIN) *.dSYM/

.PHONY: clean
