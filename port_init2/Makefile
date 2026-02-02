APP = port_start
SRC = port.c main.c rx_loop.c

PKGCONF = pkg-config

CFLAGS  += -O3 $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS += $(shell $(PKGCONF) --libs libdpdk)

$(APP): $(SRC) Makefile
	$(CC) $(CFLAGS) $(SRC) -o $(APP) $(LDFLAGS)

clean:
	rm -f $(APP)

