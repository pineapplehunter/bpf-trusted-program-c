# BPF Trusted Program (C)

eBPF LSM program using libbpf that hooks `SEC("lsm.s/bprm_creds_from_file")`
to log executed binaries and their `security.bpf.trust` xattr via ring buffer.

## Architecture

- `trust-filter.bpf.c` — eBPF program (runs in kernel). Uses
  `SEC("lsm.s/bprm_creds_from_file")` to intercept `execve()`. Reads
  `linux_binprm->filename` for the binary path, then calls
  `bpf_get_file_xattr(file, "security.bpf.trust", ...)` via kfunc and sends
  result to userspace through a ring buffer map.
- `trust-filter.c` — Userspace loader. Opens the compiled BPF `.o`,
  loads it via libbpf, attaches via BTF, polls the ring buffer for events.
- `vmlinux.h` — CO-RE kernel type definitions. Regenerate with
  `bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h`.

## Commands

| Command | What |
|---------|------|
| `make` / `make all` | Build eBPF + userspace |
| `make run` | `sudo ./trust-filter ./trust-filter.bpf.o` |
| `make clean` | Remove build artifacts |
| `nix develop` | Dev shell with clang, libbpf, bpftool |

## Runtime Requirements

- Kernel with `CONFIG_BPF_LSM=y`, booted with `lsm=bpf` (or `CONFIG_LSM="...,bpf"`)
- BTF available (`CONFIG_DEBUG_INFO_BPF=y`, checked at `/sys/kernel/btf/vmlinux`)
- `sudo` required (BPF LSM loading needs root)
- `libbpf` installed on the system (or provided via Nix shell)

## vmlinux.h

The provided `vmlinux.h` contains full kernel BTF types. To regenerate:

```
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
```
