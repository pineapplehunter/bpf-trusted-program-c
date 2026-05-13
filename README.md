# BPF Trusted Program (C)

eBPF LSM program using libbpf that hooks `SEC("lsm.s/bprm_creds_from_file")`
to log executed binaries and their `security.bpf.trust` xattr via ring buffer.

## Build (Ubuntu / Debian)

```sh
sudo apt install build-essential clang libbpf-dev pkg-config
make
```

## Run

The kernel must have BPF LSM enabled and BTF available.

**Check prerequisites:**

```sh
cat /sys/kernel/security/lsm      # must include "bpf"
ls /sys/kernel/btf/vmlinux        # must exist
```

If `bpf` is missing from the LSM list, add `lsm=bpf` to the kernel command
line and reboot.

**Start the loader:**

```sh
sudo ./trust-filter ./trust-filter.bpf.o
```

The loader prints events via stdout. To test, set the `security.bpf.trust`
xattr on a binary in another terminal:

```sh
sudo setfattr -n security.bpf.trust -v "trusted" /usr/bin/touch
touch /tmp/test
```

The loader will print:

```
Exec: "/usr/bin/touch" trust="trusted"
Exec: "/usr/bin/ls" (no xattr)
```

## NixOS

```sh
nix develop -c make run
```

## Commands

| Command | What |
|---------|------|
| `make` | Build eBPF + userspace |
| `make run` | `sudo ./trust-filter ./trust-filter.bpf.o` |
| `make clean` | Remove build artifacts |

## Files

| File | Purpose |
|------|---------|
| `trust-filter.bpf.c` | eBPF program — hooks `bprm_creds_from_file` |
| `trust-filter.c` | Userspace loader — loads BPF, reads ring buffer |
| `vmlinux.h` | Kernel BTF type definitions |
| `Makefile` | Build rules |
