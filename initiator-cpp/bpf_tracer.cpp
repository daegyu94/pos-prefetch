#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>

#include "bpf_tracer.h"
#include "metadata.h"
#include "stat.h"

#define DMFP_BPF_INFO
#ifdef DMFP_BPF_INFO
#define dmfp_bpf_info(str, ...) \
    printf("[INFO] BPFTracer::%s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_bpf_info(str, ...) do {} while (0)
#endif

//#define DMFP_BPF_DEBUG
#ifdef DMFP_BPF_DEBUG
#define dmfp_bpf_debug(str, ...) \
    printf("[DEBUG] BPFTracer::%s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_bpf_debug(str, ...) do {} while (0)
#endif

EventQueue *BPFTracer::_event_queue = nullptr;
std::unordered_map<uint64_t, std::string> BPFTracer::_entries;

const std::string bpf_mount_umount = R"(
#include <uapi/linux/ptrace.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>

//BPF_HASH(mnt_map, u32, u8, 1024);

struct mount_event_info {
    u32 dev_id;
    char dev_name[DISK_NAME_LEN];
};

BPF_PERF_OUTPUT(mount_events);

int traceret_mount_bdev(struct pt_regs *ctx)
{
    struct dentry *dentry = (struct dentry *) PT_REGS_RC(ctx);
    struct super_block *sb = dentry->d_sb;

    if (dentry) {
        struct mount_event_info info = {};
        char *dev_name = sb->s_bdev->bd_disk->disk_name;
        u32 dev_id = sb->s_dev;
        u8 value = 1;

        mnt_map.update(&dev_id, &value);

        info.dev_id = dev_id;

        bpf_probe_read_kernel_str(&info.dev_name, sizeof(info.dev_name), dev_name);
        mount_events.perf_submit(ctx, &info, sizeof(info));
    }

    return 0;
}

int trace_generic_shutdown_super(struct pt_regs *ctx, struct super_block *sb)
{
    struct mount_event_info info;
    u32 dev_id = sb->s_dev;

    mnt_map.delete(&dev_id);
    
    info.dev_id = dev_id;
    
    mount_events.perf_submit(ctx, &info, sizeof(info.dev_id));

    return 0;
}
)";

const std::string bpf_open_close = R"(
#include <linux/namei.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>
#include <linux/dcache.h>
#include <linux/fs.h>

BPF_HASH(mnt_map, u32, u8, 1024);

#define MAX_ENTRIES 32

enum event_type {
    EVENT_OPEN_ENTRY = 0,
    EVENT_OPEN_END,
    EVENT_CLOSE,
    EVENT_PAGE_DELETION,
    EVENT_PAGE_REFERENCED,
    EVENT_EXT4_MPAGE_READPAGES,
    EVENT_VFS_UNLINK,
};

enum filepath_type {
    FPATH_ABSOLUTE = 0,
    FPATH_ABSOLUTE_LONG,
    FPATH_RELATIVE,
};

struct val_t {
    u64 id;
    char comm[TASK_COMM_LEN];
    const char *fname;
};

struct data_t {
    u64 id;
    u64 ino;
    u32 dev_id;
    char comm[TASK_COMM_LEN];
    enum event_type type;
    enum filepath_type fpath_type;
    char name[NAME_MAX];
};

struct open_flags {  
    int open_flag;   
    umode_t mode;    
    int acc_mode;    
    int intent;      
    int lookup_flags;
};                   

BPF_PERF_OUTPUT(open_events);
BPF_HASH(infotmp, u64, struct val_t);

int trace_do_filp_open(struct pt_regs *ctx, 
    int dfd, struct filename *pathname, const struct open_flags *op)
{
    struct val_t val = {};
    u64 id = bpf_get_current_pid_tgid();
    
#if 0
    if (container_should_be_filtered()) {
        return 0;
    }
#endif 
    
    // TODO: ignore lookup directory 

    if (bpf_get_current_comm(&val.comm, sizeof(val.comm)) == 0) {
        val.id = id;
        val.fname = pathname->name;
        infotmp.update(&id, &val);
    }

    return 0;
}

int traceret_do_filp_open(struct pt_regs *ctx)
{
    struct file *file = (struct file *) PT_REGS_RC(ctx);
    u64 id = bpf_get_current_pid_tgid();
    struct val_t *valp;
    u32 dev_id = file->f_inode->i_sb->s_dev;
    u8 *ret;
    bool is_fpath_abs_long = false;

    valp = infotmp.lookup(&id);
    if (valp == 0) {
        // missed entry
        return 0;
    }
    
    ret = mnt_map.lookup(&dev_id);
    if (!ret) {
        // not target dev
        return 0;
    }

    struct data_t data = {};
    bpf_probe_read_kernel(&data.comm, sizeof(data.comm), valp->comm);
    bpf_probe_read_kernel_str(&data.name, sizeof(data.name), (void *) valp->fname); // TODO: ??!! how to handle absoulte long?
    data.id = valp->id;
    data.ino = file->f_inode->i_ino;
    data.dev_id = dev_id;

    if (data.name[0] == '/' && data.name[NAME_MAX - 1]) { 
        is_fpath_abs_long = true; 
    }

    data.type = EVENT_OPEN_ENTRY;
    data.fpath_type = FPATH_ABSOLUTE;
    open_events.perf_submit(ctx, &data, sizeof(data));

    if (data.name[0] != '/') { // relative path
        struct task_struct *task;
        struct dentry *dentry;
        int i;

        task = (struct task_struct *)bpf_get_current_task();
        dentry = task->fs->pwd.dentry;

        for (i = 0; i < MAX_ENTRIES; i++) {
            bpf_probe_read_kernel(&data.name, sizeof(data.name), (void *)dentry->d_name.name);
            data.type = EVENT_OPEN_ENTRY;
            open_events.perf_submit(ctx, &data, sizeof(data));

            if (dentry == dentry->d_parent) { // root directory (until mntpnt, not the actual root)
                break;
            }

            dentry = dentry->d_parent;
        }
        data.fpath_type = FPATH_RELATIVE;
    }

    if (is_fpath_abs_long) {
        int i;
        const char *ptr = valp->fname + NAME_MAX;
        for (i = 1; i < MAX_ENTRIES; i++) {
            bpf_probe_read_kernel(&data.name, sizeof(data.name), (void *) ptr);
            data.type = EVENT_OPEN_ENTRY;
            open_events.perf_submit(ctx, &data, sizeof(data));

            if (data.name[NAME_MAX - 1] == 0) {
                break;
            }
            ptr += NAME_MAX;
        }
        data.fpath_type = FPATH_ABSOLUTE_LONG;
    }

    data.type = EVENT_OPEN_END;
    open_events.perf_submit(ctx, &data, sizeof(data));

    infotmp.delete(&id);

    return 0;
}

// XXX: do not trace close
int trace_filp_close(struct pt_regs *ctx, struct file *filp, fl_owner_t id)
{  
#if 0
    u32 dev_id = filp->f_inode->i_sb->s_dev;
    u8 *ret;
    
    ret = mnt_map.lookup(&dev_id);
    if (!ret) {
        // not target dev
        return 0;
    }
    
    struct data_t data = {};
    data.type = EVENT_CLOSE;
    data.ino = filp->f_inode->i_ino; 
    data.dev_id = dev_id;
    open_events.perf_submit(ctx, &data, sizeof(data));
#endif
    return 0;
}
)";

const std::string bpf_page_deletion = R"(
#include <uapi/linux/ptrace.h>
#include <linux/page-flags.h>
#include <linux/fs.h>

struct page_event_info {
    u16 type;
    u32 dev_id;
    u64 ino;
    u64 index;
    u64 file_size;
    u32 readahead_bitmap;
    u16 readahead_size;
};

BPF_PERF_OUTPUT(page_access_events);

int trace_unaccount_page_cache_page(struct pt_regs *ctx, struct address_space *mapping, 
    struct page *page)
{
    /* check file-backed clean page */
    if (!(page->flags & (1UL << PG_uptodate)) || 
            !(page->flags & (1UL << PG_mappedtodisk))) {
            return -1;
    }
    
    struct inode *inode = mapping->host;
    struct page_event_info info = {};
    u32 dev_id = inode->i_sb->s_dev;
    u8 *ret;

    ret = mnt_map.lookup(&dev_id);
    if (!ret) {
        return -1;
    }

    info.type = EVENT_PAGE_DELETION;
    info.dev_id = dev_id; 
    info.ino = inode->i_ino;
    info.index = page->index;
    info.file_size = inode->i_size; 
    page_access_events.perf_submit(ctx, &info, sizeof(info));

    return 0;
}
)";

const std::string bpf_mark_page_accessed = R"(
int trace_mark_page_accessed(struct pt_regs *ctx, struct page *page)
{
    if (!(page->flags & (1UL << PG_uptodate)) || 
            !(page->flags & (1UL << PG_mappedtodisk))) {

            return -1;
    }

    struct inode *inode = page->mapping->host;
    struct page_event_info info = {};
    u32 dev_id = inode->i_sb->s_dev;
    u8 *ret;

    ret = mnt_map.lookup(&dev_id);
    if (!ret) {
        return 0;
    }

    info.type = EVENT_PAGE_REFERENCED;
    info.dev_id = dev_id; 
    info.ino = inode->i_ino;
    info.index = page->index;
    info.file_size = inode->i_size; 
    
    page_access_events.perf_submit(ctx, &info, sizeof(info));
    
    return 0;  
}
)";

const std::string bpf_ext4_mpage_readpages = R"(
#define fblktrace_container_of(ptr, type, member) ({                    \
        const typeof(((type *)0)->member) * __mptr = (ptr);             \
        (type *)((char *)__mptr - offsetof(type, member)); })

int trace_ext4_mpage_readpages(struct pt_regs *ctx, struct address_space *mapping,
                         struct list_head *pages, struct page *page,
                         unsigned nr_pages, bool is_readahead)
{
    int i;
    unsigned long ino = mapping->host->i_ino;
    struct page_event_info info = {};
    u32 dev_id = mapping->host->i_sb->s_dev;
    u8 *ret;

    ret = mnt_map.lookup(&dev_id);
    if (!ret) {
        return -1;
    }
    
    if (page->index >= (mapping->host->i_size >> 12)) { 
        return -1;
    }
    
    info.type = EVENT_EXT4_MPAGE_READPAGES;
    info.dev_id = dev_id;
    info.ino = mapping->host->i_ino;
    info.file_size = mapping->host->i_size;
    
    #pragma unroll
    for (i = 0; i < 32 && nr_pages--; i++) {
        if (pages) {
            pages = pages->prev;
            page = fblktrace_container_of(pages, struct page, lru);
        }

        info.readahead_bitmap |= (1U << i);
        info.readahead_size++;
    }

    page_access_events.perf_submit(ctx, &info, sizeof(info));
    return 0;
}
)";

const std::string bpf_vfs_unlink = R"(
int trace_vfs_unlink(struct pt_regs *ctx, struct inode *dir, 
    struct dentry *dentry, struct inode **delegated_inode)
{
    u32 dev_id = dentry->d_inode->i_sb->s_dev;
    u8 *ret;
    struct page_event_info info = {};
    
    ret = mnt_map.lookup(&dev_id);
    if (!ret) {
        return -1;
    }
    
    info.type = EVENT_VFS_UNLINK;
    info.dev_id = dev_id;
    info.ino = dentry->d_inode->i_ino;

    page_access_events.perf_submit(ctx, &info, sizeof(info));
    
    return 0;
}
)";

enum filepath_type {
    FPATH_ABSOLUTE = 0,
    FPATH_ABSOLUTE_LONG,
    FPATH_RELATIVE,
};

/*
struct mount_event_info {
    uint32_t dev_id;
    char dev_name[32];
};
*/

struct OpenEvent {
    uint64_t id;
    uint64_t ino;
    uint32_t dev_id;
    char comm[16];
    enum event_type type;
    enum filepath_type fpath_type;
    char name[255];
};

void BPFTracer::HandleOpenEvents(void *cb_cookie, void *data, int data_size) {
    auto event = static_cast<OpenEvent *>(data);
    uint32_t dev_id = event->dev_id;
    uint64_t ino = event->ino;
    uint64_t tgid = event->id;
    std::string name(event->name);

    if (event->type == EVENT_OPEN_END) {
        auto paths_it = _entries.find(tgid);
        if (paths_it == _entries.end()) {
            return;
        }
        
        /* XXX: only support absoulte path, don't have to consider other things */
        filepath_map.Put(std::make_pair(event->dev_id, event->ino), 
                std::make_pair(event->fpath_type, name));
        
        dmfp_bpf_debug("%s\n", name.c_str());
        
        /* TODO: check result */
        size_t erasedCount = _entries.erase(tgid);
        
    } else if (event->type == EVENT_OPEN_ENTRY) {
        _entries[tgid] = name;
    } else if (event->type == EVENT_CLOSE) {

    } else {
        printf("[ERROR] Unknown event type\n");
    }
}

/* allocate new object and free when dequeue */
void BPFTracer::HandlePageAccessEvents(void *cb_cookie, void *data, int data_size) {
    auto ev = static_cast<BPFEvent *>(data);
    BPFEvent *event = new BPFEvent;

    memcpy(event, ev, sizeof(BPFEvent));
    
    dmfp_bpf_debug("dev_id=%u, ino=%lu, index=%lu, file_size=%lu, "
            "readahead_bitmap=%u, readahead_size=%u, type=%u\n", 
            event->dev_id, event->ino, event->index, event->file_size, 
            event->readahead_bitmap, event->readahead_size, event->type);
 
    _event_queue->Enqueue((void *) event);
}

void BPFTracer::HandleLostOpenEvents(void *cb_cookie, uint64_t lost) {
    counter.bpf_lost_open += lost;
}

void BPFTracer::HandleLostPageAccessEvents(void *cb_cookie, uint64_t lost) {
    counter.bpf_lost_page_access += lost;
}

void BPFTracer::HandleLostReadpagesEvents(void *cb_cookie, uint64_t lost) {
    counter.bpf_lost_readpages += lost;
}

BPFTracer::BPFTracer() {
    std::string BPF_PROGRAM = bpf_open_close + \
                              bpf_page_deletion + \
                              bpf_mark_page_accessed + \
                              bpf_ext4_mpage_readpages + \
                              bpf_vfs_unlink;

    dmfp_bpf_info("\n");
    
    auto init_res = _bpf.init(BPF_PROGRAM);
    if (!init_res.ok()) {
        std::cerr << init_res.msg() << std::endl;
        return;
    }
    
    /*
    auto attach_res = _bpf.attach_kprobe("mount_bdev", "traceret_mount_bdev", 0,
            BPF_PROBE_RETURN, 0);
    if (!attach_res.ok()) {
        std::cerr << attach_res.msg() << std::endl;
        return;
    }

    attach_res = _bpf.attach_kprobe("generic_shutdown_super", 
            "trace_generic_shutdown_super", 0, BPF_PROBE_ENTRY, 0);
    if (!attach_res.ok()) {
        std::cerr << attach_res.msg() << std::endl;
        return;
    }
    */

    auto attach_res = _bpf.attach_kprobe("do_filp_open", 
            "trace_do_filp_open", 0, BPF_PROBE_ENTRY, 0);
    if (!attach_res.ok()) {
        std::cerr << attach_res.msg() << std::endl;
        return;
    }
    attach_res = _bpf.attach_kprobe("do_filp_open", 
            "traceret_do_filp_open", 0, BPF_PROBE_RETURN, 0);
    if (!attach_res.ok()) {
        std::cerr << attach_res.msg() << std::endl;
        return;
    }

    /*
    attach_res = _bpf.attach_kprobe("filp_close", 
            "trace_filp_close", 0, BPF_PROBE_ENTRY, 0);
    if (!attach_res.ok()) {
        std::cerr << attach_res.msg() << std::endl;
        return;
    }
    */
    attach_res = _bpf.attach_kprobe("unaccount_page_cache_page", 
            "trace_unaccount_page_cache_page", 0, BPF_PROBE_ENTRY, 0);
    if (!attach_res.ok()) {
        std::cerr << attach_res.msg() << std::endl;
        return;
    }

    attach_res = _bpf.attach_kprobe("mark_page_accessed", 
            "trace_mark_page_accessed", 0, BPF_PROBE_ENTRY, 0);
    if (!attach_res.ok()) {
        std::cerr << attach_res.msg() << std::endl;
        return;
    }
    attach_res = _bpf.attach_kprobe("ext4_mpage_readpages", 
            "trace_ext4_mpage_readpages", 0, BPF_PROBE_ENTRY, 0);
    if (!attach_res.ok()) {
        std::cerr << attach_res.msg() << std::endl;
        return;
    }
    attach_res = _bpf.attach_kprobe("vfs_unlink", 
            "trace_vfs_unlink", 0, BPF_PROBE_ENTRY, 0);
    if (!attach_res.ok()) {
        std::cerr << attach_res.msg() << std::endl;
        return;
    }

    /* TODO: lost_cb, nullptr, page_cnt */
    auto open_res = _bpf.open_perf_buffer("open_events", &HandleOpenEvents, 
            nullptr, nullptr, 512);
    if (!open_res.ok()) {
        std::cerr << open_res.msg() << std::endl;
        return;
    }
    open_res = _bpf.open_perf_buffer("page_access_events", 
            &HandlePageAccessEvents, nullptr, nullptr, 2048);
    if (!open_res.ok()) {
        std::cerr << open_res.msg() << std::endl;
        return;
    }
     
    _thd = std::thread(&BPFTracer::Run, this);
}

void BPFTracer::UpdateMntMap(uint32_t dev_id) {
    ebpf::BPFHashTable<uint32_t, uint8_t> mnt_map = 
        _bpf.get_hash_table<uint32_t, uint8_t>("mnt_map");

    uint32_t key = dev_id;
    uint8_t value = 1;
    auto res = mnt_map.update_value(key, value);
    if (!res.ok()) {
        printf("Failed to update mnt_map, dev_id=%u, err_msg=%s\n", 
                dev_id, res.msg().c_str()); 
    } else {
        dmfp_bpf_info("update mnt_map, dev_id=%u\n", dev_id); 
    }
} 

void BPFTracer::Wait(void) {
    if (_thd.joinable()) {
        _thd.join();
    }
}

void BPFTracer::Run() {
    auto open_pb = _bpf.get_perf_buffer("open_events");
    auto page_access_pb = _bpf.get_perf_buffer("page_access_events");
        
    dmfp_bpf_info("start to trace kernel events\n");

    if (open_pb && page_access_pb) {
        while (true) {
            open_pb->poll(0);
            page_access_pb->poll(1);
        }
    } else {
        printf("[ERROR] Failed to poll perf buffer\n");
    }
}
