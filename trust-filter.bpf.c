// SPDX-License-Identifier: GPL-2.0
#include "trust-filter-common.h"
#include "vmlinux.h"
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "GPL";

#ifndef ENODATA
#define ENODATA 61
#endif

struct {
  __uint(type, BPF_MAP_TYPE_RINGBUF);
  __uint(max_entries, 1 << 10);
} rb SEC(".maps");

char xattr_buf[64];

SEC("lsm.s/bprm_creds_from_file")
int BPF_PROG(bprm_creds_from_file, struct linux_binprm *bprm,
             struct file *file) {
  struct bpf_dynptr dynptr = {0};
  struct event *e = {0};
  long ret_rd = 0;
  long ret = 0;

  e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
  if (!e)
    return 0;

  e->pid = bpf_get_current_pid_tgid() >> 32;

  ret = bpf_probe_read_kernel_str(e->filename, sizeof(e->filename),
                                  BPF_CORE_READ(bprm, filename));
  if (ret <= 1) {
    bpf_ringbuf_submit(e, 0);
    return 0;
  }

  if (!file) {
    e->xattr_len = -ENODATA;
    bpf_ringbuf_submit(e, 0);
    return 0;
  }

  bpf_dynptr_from_mem(xattr_buf, sizeof(xattr_buf), 0, &dynptr);
  ret = bpf_get_file_xattr(file, "user.my", &dynptr);
  if (ret > 0) {
    if (ret >= (long)sizeof(xattr_buf))
      ret = sizeof(xattr_buf) - 1;
    ret_rd = bpf_probe_read_kernel(e->xattr_value, ret, xattr_buf);
    if (!ret_rd)
      e->xattr_len = ret;
    else
      e->xattr_len = ret_rd;
  } else {
    e->xattr_len = ret;
  }
  bpf_ringbuf_submit(e, 0);

  return 0;
}
