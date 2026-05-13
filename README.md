# BPF Trusted Program (C)

eBPF LSM program using libbpf that hooks `SEC("lsm.s/bprm_creds_from_file")`
to log executed binaries and their `security.bpf.trust` xattr via ring buffer.

## Build (Ubuntu / Debian)

```sh
sudo apt install build-essential clang libbpf-dev pkg-config
make
```

## Run

### Prerequisites

The kernel must have BPF LSM enabled with **both** of these checks passing:

```sh
cat /sys/kernel/security/lsm        # must include "bpf"
ls /sys/kernel/btf/vmlinux          # must exist (BTF)
```

If `bpf` is missing, check whether your kernel has BPF LSM compiled in:

```sh
zgrep CONFIG_BPF_LSM /proc/config.gz
```

Common distributions that ship with `CONFIG_BPF_LSM=y`:
- Ubuntu 23.04+ (but needs boot parameter)
- Fedora 38+
- Arch Linux
- NixOS (with `boot.kernelParams = [ "lsm=bpf" ]`)

**To enable BPF LSM on Ubuntu/Debian:**

modify the grub config in `/etc/default/grub`

```
...
GRUB_CMDLINE_LINUX="lsm=lockdown,capability,landlock,yama,apparmor,ima,evm,bpf"
...
```

And update grub.

```sh
# Add lsm=bpf to kernel cmdline
sudo update-grub
sudo reboot
# Verify after reboot:
cat /sys/kernel/security/lsm
```

### Start the loader

```sh
sudo ./trust-filter ./trust-filter.bpf.o
```

The loader prints events to stdout. In another terminal, set the
`user.trust` xattr on a binary to test:

```sh
setfattr -n user.trust -v "trusted" /usr/bin/touch
touch /tmp/test
```

Expected output:

```
Exec: "/usr/bin/touch" trust="trusted"
Exec: "/usr/bin/ls" (no xattr)
```

The xattr name `user.trust` is the only xattr this program reads.
Setting it marks the binary as "trusted" by the BPF LSM policy.

### Read the xattr directly

```sh
getfattr -n user.trust /usr/bin/touch
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
| `bpftool btf dump ...` | Regenerate `vmlinux.h` |

## Files

| File | Purpose |
|------|---------|
| `trust-filter.bpf.c` | eBPF program — hooks `SEC("lsm.s/bprm_creds_from_file")` |
| `trust-filter.c` | Userspace loader — loads BPF, polls ring buffer |
| `vmlinux.h` | Kernel BTF type definitions |
| `Makefile` | Build rules |
