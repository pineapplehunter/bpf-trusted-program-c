BPF_CC ?= clang
CC ?= gcc
PKG_CONFIG ?= pkg-config
CFLAGS ?= -g -O2 -std=c23 -Wall -Wextra -lbpf
BPF_CFLAGS ?= -g -O2 -std=gnu17 -Wall -Wextra -Wno-missing-declarations -Wno-unused-parameter $(shell pkg-config --cflags libbpf)

COMMON_FILES = trust-filter-common.h

.PHONY: all clean run

all: trust-filter.bpf.o trust-filter

%.bpf.o: %.bpf.c vmlinux.h $(COMMON_FILES)
	$(BPF_CC) -o $@ -c $< -target bpf $(BPF_CFLAGS)

%.o: %.c $(COMMON_FILES)
	$(CC) -o $@ -c $< $(CFLAGS)

trust-filter: trust-filter.o
	$(CC) -o $@ $^ $(CFLAGS)

vmlinux.h:
	bpftool btf dump file /sys/kernel/btf/vmlinux format c > $@

run: all
	sudo ./trust-filter ./trust-filter.bpf.o

clean:
	$(RM) -f *.o trust-filter
