// SPDX-License-Identifier: GPL-2.0
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

char LICENSE[] SEC("license") = "GPL";

struct event {
    __u32 pid;
    __s32 xattr_len;
    char filename[64];
    char xattr_value[8];
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24);
} rb SEC(".maps");

SEC("lsm/bprm_check_security")
int BPF_PROG(bprm_check_security, struct linux_binprm *bprm)
{
    struct file *file_ptr;
    struct inode *inode_ptr;
    struct event *e;
    long ret;

    e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
    if (!e)
        return 0;

    e->pid = bpf_get_current_pid_tgid() >> 32;
    e->xattr_len = 0;

    ret = bpf_probe_read_kernel_str(e->filename, sizeof(e->filename),
                    BPF_CORE_READ(bprm, filename));
    if (ret <= 1) {
        bpf_ringbuf_submit(e, 0);
        return 0;
    }

    file_ptr = BPF_CORE_READ(bprm, file);
    if (!file_ptr) {
        bpf_ringbuf_submit(e, 0);
        return 0;
    }

    inode_ptr = BPF_CORE_READ(file_ptr, f_inode);
    if (!inode_ptr) {
        bpf_ringbuf_submit(e, 0);
        return 0;
    }

    ret = bpf_probe_read_kernel(e->xattr_value, sizeof(e->xattr_value),
                    BPF_CORE_READ(inode_ptr, i_security));
    if (!ret) {
        e->xattr_len = sizeof(e->xattr_value);
    } else {
        e->xattr_len = ret;
    }
    bpf_ringbuf_submit(e, 0);

    return 0;
}
