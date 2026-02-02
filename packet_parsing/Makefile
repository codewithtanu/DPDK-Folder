APP = process_packet
SRC = port_init.c main.c packet_process.c mem_pool.c mbuf_info.c

PKGCONF = pkg-config

CFLAGS  += -O3 $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS += $(shell $(PKGCONF) --libs libdpdk)

$(APP): $(SRC) Makefile
	$(CC) $(CFLAGS) $(SRC) -o $(APP) $(LDFLAGS)

clean:
	rm -f $(APP)
