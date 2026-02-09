APP1 = prim
APP2 = sec

SRCS-y1 := prim.c
SRCS-y2 := sec.c

PKGCONF ?= pkg-config

PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += -O3 $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk)

build: $(APP1) $(APP2)

$(APP1): $(SRCS-y1) Makefile $(PC_FILE)
	$(CC) $(CFLAGS) $(SRCS-y1) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

$(APP2): $(SRCS-y2) Makefile $(PC_FILE)
	$(CC) $(CFLAGS) $(SRCS-y2) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

clean:
	rm -f $(APP1) $(APP2)

.PHONY: build clean