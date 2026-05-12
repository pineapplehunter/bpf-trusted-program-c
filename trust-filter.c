#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

static volatile sig_atomic_t exiting = 0;

struct event {
    __u32 pid;
    __s32 xattr_len;
    char filename[64];
    char xattr_value[8];
};

static void sig_handler(int sig)
{
    exiting = 1;
}

static int handle_event(void *ctx, void *data, size_t size)
{
    struct event *e = data;

    if (e->xattr_len == 8) {
        printf("Exec: \"%s\" i_sec=%02x%02x%02x%02x%02x%02x%02x%02x\n",
               e->filename,
               (unsigned char)e->xattr_value[0],
               (unsigned char)e->xattr_value[1],
               (unsigned char)e->xattr_value[2],
               (unsigned char)e->xattr_value[3],
               (unsigned char)e->xattr_value[4],
               (unsigned char)e->xattr_value[5],
               (unsigned char)e->xattr_value[6],
               (unsigned char)e->xattr_value[7]);
    } else {
        printf("Exec: \"%s\" (xattr err=%d)\n", e->filename, e->xattr_len);
    }
    fflush(stdout);
    return 0;
}

int main(int argc, char **argv)
{
    struct bpf_object *obj = NULL;
    struct bpf_program *prog = NULL;
    struct bpf_link *link = NULL;
    struct ring_buffer *rb = NULL;
    const char *filename;
    int rb_map_fd;
    int err;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bpf_program.o>\n", argv[0]);
        return 1;
    }
    filename = argv[1];

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    obj = bpf_object__open_file(filename, NULL);
    if (!obj) {
        fprintf(stderr, "Failed to open BPF object file '%s'\n", filename);
        return 1;
    }

    prog = bpf_object__find_program_by_name(obj, "bprm_check_security");
    if (!prog) {
        fprintf(stderr, "Failed to find 'bprm_check_security' program\n");
        goto cleanup;
    }

    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %s\n", strerror(-err));
        goto cleanup;
    }

    link = bpf_program__attach(prog);
    if (!link) {
        fprintf(stderr, "Failed to attach LSM program: %s\n",
            strerror(-errno));
        goto cleanup;
    }

    rb_map_fd = bpf_object__find_map_fd_by_name(obj, "rb");
    if (rb_map_fd < 0) {
        fprintf(stderr, "Failed to find ring buffer map\n");
        goto cleanup;
    }

    rb = ring_buffer__new(rb_map_fd, handle_event, NULL, NULL);
    if (!rb) {
        fprintf(stderr, "Failed to create ring buffer\n");
        goto cleanup;
    }

    printf("Loaded. Waiting for Ctrl-C...\n");

    while (!exiting)
        ring_buffer__poll(rb, 200);

    printf("Exiting...\n");

cleanup:
    ring_buffer__free(rb);
    bpf_link__destroy(link);
    bpf_object__close(obj);
    return 0;
}
