from bcc import BPF
from bcc.utils import printb
from collections import defaultdict
import os
from ctypes import *

import threading
import queue 

from metadata import *
from stats import *
from my_types import *

bpf_text_mount = """
#include <uapi/linux/ptrace.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>

BPF_HASH(mnt_map, u32, u8, 1024);

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
"""

bpf_text_open = """
#include <linux/namei.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>
#include <linux/dcache.h>
#include <linux/fs.h>

#define MAX_ENTRIES 32

enum event_type {
    EVENT_OPEN_ENTRY = 0,
    EVENT_OPEN_END,
    EVENT_CLOSE,
    EVENT_PAGE_DELETION,
    EVENT_PAGE_REFERENCED,
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
"""

bpf_text_page_deletion = """
#include <uapi/linux/ptrace.h>
#include <linux/page-flags.h>
#include <linux/fs.h>

struct page_event_info {
    u32 type;
    u32 dev_id;
    u64 ino;
    u64 index;
    u64 file_size;
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
"""

bpf_text_mark_page_accessed = """
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
"""

bpf_text_vfs_unlink = """
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
"""

bpf_text_ext4_mpage_readpages = """
struct readpages_data_t {
    u32 dev_id;
    u64 file_size;
    u64 ino;
    u64 indexes[32];
    u8 is_readaheads[32];
    u32 size;
};

BPF_PERF_OUTPUT(ext4_mpage_readpages_events);

#define fblktrace_container_of(ptr, type, member) ({                    \
        const typeof(((type *)0)->member) * __mptr = (ptr);             \
        (type *)((char *)__mptr - offsetof(type, member)); })

int trace_ext4_mpage_readpages(struct pt_regs *ctx, struct address_space *mapping,
                         struct list_head *pages, struct page *page,
                         unsigned nr_pages, bool is_readahead)
{
    int i;
    unsigned long ino = mapping->host->i_ino;
    struct readpages_data_t data = {};
    u32 dev_id = mapping->host->i_sb->s_dev;
    u8 *ret;
    u32 size = 0;

    ret = mnt_map.lookup(&dev_id);
    if (!ret) {
        return -1;
    }

    data.dev_id = dev_id;
    data.file_size = mapping->host->i_size;
    data.ino = mapping->host->i_ino;

    if (page->index >= (mapping->host->i_size >> 12)) { 
        return -1;
    }
    
    #pragma unroll
    for (i = 0; i < 32 && nr_pages--; i++) {
        if (pages) {
            pages = pages->prev;
            page = fblktrace_container_of(pages, struct page, lru);
        }
        data.indexes[i] = page->index;
        data.is_readaheads[i] = is_readahead;
        
        size++;
    }
    data.size = size;

    ext4_mpage_readpages_events.perf_submit(ctx, &data, sizeof(data));
    return 0;
}
"""

bpf_prog = bpf_text_mount + bpf_text_open + bpf_text_page_deletion + \
        bpf_text_ext4_mpage_readpages + bpf_text_mark_page_accessed + bpf_text_vfs_unlink

bpf = BPF(text=bpf_prog)

mount_fnname = "mount_bdev"
umount_fnname = "generic_shutdown_super"

open_fname = "do_filp_open"
close_fname = "filp_close"

page_deletion_fname = "unaccount_page_cache_page"

bpf.attach_kretprobe(event = mount_fnname, 
        fn_name = "traceret_" + mount_fnname)
bpf.attach_kprobe(event = umount_fnname, 
        fn_name = "trace_" + umount_fnname)

bpf.attach_kprobe(event = open_fname, fn_name = "trace_" + open_fname)
bpf.attach_kretprobe(event = open_fname, fn_name = "traceret_" + open_fname)
bpf.attach_kprobe(event = close_fname, fn_name = "trace_" + close_fname)

bpf.attach_kprobe(event = page_deletion_fname, 
        fn_name = "trace_" + page_deletion_fname)

bpf.attach_kprobe(event = "mark_page_accessed", 
        fn_name = "trace_mark_page_accessed")

bpf.attach_kprobe(event = "vfs_unlink", 
        fn_name = "trace_vfs_unlink")

ext4_mpage_readpages_fname = "ext4_mpage_readpages"
bpf.attach_kprobe(event = ext4_mpage_readpages_fname, 
        fn_name = "trace_" + ext4_mpage_readpages_fname)

MAX_BUFFERED_EVENTS = 32
MAX_BUFFERED_EVENTS2 = 1024

class BPFCallback:
    def __init__(self):
        pass

    def mount_umount(self):
        event = bpf["mount_events"].event(data)

        dev_id = event.dev_id
        if event.dev_name:
            # mount
            dev_name = event.dev_name.decode('utf-8')
            pass
            #self.meta_dict.put(dev_id, dev_name)
        else:
            # umount
            pass
            #self.meta_dict.delete(dev_id)

    
    def open_close(self):
        event = bpf["open_events"].event(data)

        dev_id = event.dev_id
        ino = event.ino
        tgid = event.id
        name = event.name 
        
        if event.type == EventType.EVENT_OPEN_END:
            paths = self._entries[tgid]
            if paths is None:
                return

            path_type = FilePathType.FPATH_ABSOLUTE
            if (event.fpath_type == FilePathType.FPATH_RELATIVE):
                path_type = FilePathType.FPATH_RELATIVE
                paths.reverse()
                
                #file_path = os.path.join(*paths)
                #Stats.open_rel += 1
            elif event.fpath_type == FilePathType.FPATH_ABSOLUTE_LONG:
                file_path = b''.join(paths) # TODO: ??
                #Stats.open_abs_long += 1
            else:
                file_path = name
                #Stats.open_abs += 1

            #self._fpath_dict[(dev_id, ino)] = (path_type, file_path)

            try:
                del(self._entries[tgid])
            except Exception:
                pass

        elif event.type == EventType.EVENT_OPEN_ENTRY:
            self._entries[tgid].append(name)
            
        elif event.type == EventType.EVENT_CLOSE:
            #self._fpath_dict.pop((dev_id, ino), None)
            pass
        else:
            assert 0, "Unknown event type"


    def page_access(self):
        event = bpf["page_access_events"].event(data)

        page_info = PageAccessEvent(event.type, event.dev_id, event.ino, event.index, event.file_size)
        if event.type == EventType.EVENT_PAGE_DELETION: 
            self._event_list.append(page_info)
            self._num_events += 1
            if self._num_events == MAX_BUFFERED_EVENTS:
                try:
                    #self._event_queue.put(self._event_list, block=False)
                    Stats.page_deletion += MAX_BUFFERED_EVENTS
                except queue.Full:
                    Stats.lost_queue_full_page_deletion += MAX_BUFFERED_EVENTS
                
                self._event_list = []
                self._num_events = 0

        elif event.type == EventType.EVENT_PAGE_REFERENCED:
            self._event_list2.append(page_info)
            self._num_events2 += 1
            if self._num_events2 == MAX_BUFFERED_EVENTS2:
                try:
                    #self._event_queue.put(self._event_list2, block=False)
                    Stats.page_referenced += MAX_BUFFERED_EVENTS2
                except queue.Full:
                    pass
                    Stats.lost_queue_full_page_referenced += MAX_BUFFERED_EVENTS2
                
                self._event_list2 = []
                self._num_events2 = 0

        elif event.type == EventType.EVENT_VFS_UNLINK:
            try:
                #self._event_queue.put(page_info, block=False)
                Stats.vfs_unlink += 1
            except queue.Full:
                Stats.lost_queue_full_vfs_unlink += 1
        else:
            assert 0, "Unknown event type"
 

    def ext4_mpage_readpages(self):
        event = bpf["ext4_mpage_readpages_events"].event(data)
        u64_array = event.indexes
        u64_array_ctypes = (c_uint64 * len(u64_array)).from_address(addressof(u64_array))
        index_list = list(u64_array_ctypes)

        u8_array = event.is_readaheads
        u8_array_ctypes = (c_uint8 * len(u8_array)).from_address(addressof(u8_array))
        is_readahead_list = list(u8_array_ctypes)

        try:
            Stats.ext4_mpage_readpages += 1
            self._event_queue.put(Ext4ReadPagesEvent(event.dev_id, event.ino, 
                index_list, is_readahead_list, event.size, event.file_size), block=False)
        except queue.Full:
            Stats.lost_queue_full_ext4_mpage_readpages += 1

 
class BPFLostCallback:
    def __init__(self):
        pass

    def mount_umount(self):
        pass

    def open_close(self):
        pass

    def page_access(self):
        pass 

    def ext4_mpage_readpages(self):
        pass 

    
class BPFTracer(threading.Thread):
    def __init__(self, event_queue, meta_dict, fpath_dict):
        threading.Thread.__init__(self)
        super().__init__()
        self.name = "BPFTracer"
        
        self._event_queue = event_queue
        self._meta_dict = meta_dict
        self._fpath_dict = fpath_dict

    def run(self):
        mnt_map = bpf.get_table("mnt_map")
        dev_ids = self._meta_dict.get_dev_ids() 
        for dev_id in dev_ids:
            mnt_map[c_int(dev_id)] = c_int(1)
        
        callback = BPFCallback()
        lost_cb = BPFLostCallback()
        
        #bpf["mount_events"].open_perf_buffer(
        #        callback.mount_umount, page_cnt=2)
        
        bpf["open_events"].open_perf_buffer(
                callback.open_close, 
                page_cnt=64, 
                lost_cb=callback_open_close.lost_cb)

        bpf["page_access_events"].open_perf_buffer(
                callback.page_access, 
                page_cnt=1024,
                lost_cb=lost_cb.page_access)

        bpf["ext4_mpage_readpages_events"].open_perf_buffer(
                callback.ext4_mpage_readpages, 
                page_cnt=1024,
                lost_cb=lost_cb.ext4_mpage_readpages)

        while True:
            try:
                bpf.perf_buffer_poll()
            except KeyboardInterrupt:
                break
            except ValueError:
                continue

# TODO: test program
if __name__ == '__main__':
    pass
