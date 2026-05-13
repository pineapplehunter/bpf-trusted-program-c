#ifdef __BPF__
#include "vmlinux.h"
#else
#include <linux/types.h>
#endif

struct event {
  __u32 pid;
  __s32 xattr_len;
  char filename[128];
  char xattr_value[64];
};
