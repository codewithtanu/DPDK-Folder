
CC = gcc

# Get DPDK flags from pkg-config
DPDK_CFLAGS = $(shell pkg-config --cflags libdpdk)
DPDK_LDFLAGS = $(shell pkg-config --libs libdpdk)

CFLAGS = -Wall -Wextra -O2 $(DPDK_CFLAGS)

all: prim sec

prim: primary.c
	$(CC) $(CFLAGS) primary.c -o prim $(DPDK_LDFLAGS)

sec: secondary.c
	$(CC) $(CFLAGS) secondary.c -o sec $(DPDK_LDFLAGS)

clean:
	rm -f prim sec
