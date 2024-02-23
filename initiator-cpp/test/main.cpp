#include <iostream>
#include <unistd.h>
#include <linux/perf_event.h>
#include <bpf/libbpf.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct event_info {
    uint32_t dev_id;
    char dev_name[32];
};

int main() {
    struct perf_event_attr attr = {};
    attr.type = PERF_TYPE_SOFTWARE;
    attr.config = PERF_COUNT_SW_BPF_OUTPUT;
    attr.sample_type = PERF_SAMPLE_RAW;
    attr.sample_size = sizeof(struct event_info);
    attr.wakeup_events = 1;

    int perf_fd = sys_perf_event_open(&attr, -1, 0, -1, 0);
    if (perf_fd < 0) {
        std::cerr << "Failed to open perf event: " << strerror(errno) << std::endl;
        return 1;
    }

    std::string bpf_program = R"(
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>

struct event_info {
    u32 dev_id;
    char dev_name[32];
};

BPF_PERF_OUTPUT(events);

int kretprobe_mount_bdev(struct pt_regs *ctx)
{
    struct dentry *dentry = (struct dentry *) PT_REGS_RC(ctx);
    struct super_block *sb = dentry->d_sb;

    if (dentry) {
        struct event_info info;
        char *dev_name = (char *) PT_REGS_PARM3(ctx);

        info.dev_id = sb->s_dev;
        bpf_probe_read_kernel_str(&info.dev_name, sizeof(info.dev_name), dev_name);
        events.perf_submit(ctx, &info, sizeof(info));
    }

    return 0;
}

int kprobe_generic_shutdown_super(struct pt_regs *ctx, struct super_block *sb)
{
    struct event_info info;

    info.dev_id = sb->s_dev;
    events.perf_submit(ctx, &info, sizeof(info.dev_id));

    return 0;
}
)";

    struct bpf_prog_load_attr prog_load_attr = {};
    prog_load_attr.file = BPF_PROG_PATH;
    prog_load_attr.prog_type = BPF_PROG_TYPE_KPROBE;
    prog_load_attr.expected_attach_type = BPF_CGROUP_INET_INGRESS;
    prog_load_attr.insn_cnt = 1;
    prog_load_attr.insns = (struct bpf_insn*)bpf_program.c_str();

    int prog_fd = bpf_prog_load_xattr(&prog_load_attr, NULL, 0);
    if (prog_fd < 0) {
        std::cerr << "Failed to load BPF program: " << strerror(errno) << std::endl;
        close(perf_fd);
        return 1;
    }

    struct perf_event_mmap_page* mmap_page = (struct perf_event_mmap_page*)mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE, MAP_SHARED, perf_fd, 0);
    if (mmap_page == MAP_FAILED) {
        std::cerr << "Failed to mmap perf event: " << strerror(errno) << std::endl;
        close(perf_fd);
        return 1;
    }

    int probe_fd_mount = bpf_attach_kprobe(prog_fd, BPF_CGROUP_INET_INGRESS, "kretprobe_mount_bdev");
    int probe_fd_shutdown = bpf_attach_kprobe(prog_fd, BPF_CGROUP_INET_INGRESS, "kprobe_generic_shutdown_super");

    if (probe_fd_mount < 0 || probe_fd_shutdown < 0) {
        std::cerr << "Failed to attach BPF program to kprobe: " << strerror(errno) << std::endl;
        munmap(mmap_page, sysconf(_SC_PAGESIZE));
        close(perf_fd);
        close(prog_fd);
        return 1;
    }

    // Main loop to read perf events
    while (1) {
        int poll_ret = poll(&mmap_page->data_ready, 1, -1);
        if (poll_ret < 0) {
            std::cerr << "Failed to poll perf event: " << strerror(errno) << std::endl;
            break;
        }

        if (mmap_page->data_ready != 1) {
            continue;
        }

        uint64_t data_offset = mmap_page->data_offset;
        while (data_offset != mmap_page->data_tail) {
            struct event_info* event = (struct event_info*)((char*)mmap_page + data_offset);
            if (event->dev_name[0] != '\0') {
                std::cout << "mount: dev_id=" << event->dev_id << ", dev_name=" << event->dev_name << std::endl;
            } else {
                std::cout << "umount: dev_id=" << event->dev_id << std::endl;
            }
            data_offset += sizeof(struct event_info);
        }

        mmap_page->data_ready = 0;
    }

    close(probe_fd_mount);
    close(probe_fd_shutdown);
    munmap(mmap_page, sysconf(_SC_PAGESIZE));
    close(perf_fd);
    close(prog_fd);

    return 0;
}
