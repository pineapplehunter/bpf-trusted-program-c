# BPF Trusted Program (C)

eBPF LSM program using libbpf that hooks `bprm_check_security` to log executed
binaries and their inode security xattrs.

## Architecture

- `trust-filter.bpf.c` — eBPF program (runs in kernel). Uses
  `SEC("lsm/bprm_check_security")` to intercept `execve()`. Reads
  `linux_binprm->filename` for the binary path and traverses `file->f_inode->i_security`
  to dump the first 8 bytes of the security blob.
- `trust-filter.c` — Userspace loader. Opens the compiled BPF `.o`,
  loads it via libbpf, attaches via BTF, waits for Ctrl+C.
- `vmlinux.h` — CO-RE kernel type definitions. Regenerate with
  `bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h`.

## Commands

| Command | What |
|---------|------|
| `cmake -B build && cmake --build build` | Build eBPF + userspace |
| `cmake --build build --target run` | `sudo ./build/trust-filter ./build/trust-filter.bpf.o` |
| `rm -rf build` | Remove build artifacts |
| `nix develop` | Dev shell with cmake, clang, libbpf, ninja |

## Runtime Requirements

- Kernel with `CONFIG_BPF_LSM=y`, booted with `lsm=bpf` (or `CONFIG_LSM="...,bpf"`)
- BTF available (`CONFIG_DEBUG_INFO_BPF=y`, checked at `/sys/kernel/btf/vmlinux`)
- `sudo` required (BPF LSM loading needs root)
- `libbpf` installed on the system (or provided via Nix shell)

## vmlinux.h

The provided `vmlinux.h` contains minimal struct definitions used by the BPF
program. CO-RE relocations resolve field offsets at load time, so the
definitions only need correct field names — not exact layout.

To regenerate the full `vmlinux.h`:
```
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
```
