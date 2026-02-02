APP = mem_pool2
SRC = mem_pool2.c

PKGCONF = pkg-config

CFLAGS  += -O3 $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS += $(shell $(PKGCONF) --libs libdpdk)

$(APP): $(SRC) Makefile
	$(CC) $(CFLAGS) $(SRC) -o $(APP) $(LDFLAGS)

clean:
	rm -f $(APP)

