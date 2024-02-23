#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/fs.h>
#include <linux/fiemap.h>

#include <time.h>
#include <stdint.h>

#include "fiemap.h"
#include "stat.h"

void showExtents(struct fiemap *fiemap) {
    printf("#Extent\tLogical\tPhysical\tLength\tFlags\n");
    for (int i = 0; i < fiemap->fm_mapped_extents; i++) {
        printf("%d:\t0x%llx\t0x%llx\t%lld\t%d\n",
                i,
                fiemap->fm_extents[i].fe_logical,
                fiemap->fm_extents[i].fe_physical,
                fiemap->fm_extents[i].fe_length,
                fiemap->fm_extents[i].fe_flags);
    }
}

int doFiemap(int fd, std::vector<Extent *> &vec) {
    struct fiemap *fiemap = new struct fiemap;
    if (!fiemap) {
        fprintf(stderr, "Out of memory allocating fiemap\n");	
        return -1;
    }
    
    fiemap->fm_start = 0;
    fiemap->fm_length = ~0; /* Lazy way, this is filled after ioctl */
    fiemap->fm_flags = FIEMAP_FLAG_SYNC;
    fiemap->fm_extent_count = 0;
    fiemap->fm_mapped_extents = 0;

    /* 1. Find out how many extents there are, submit ioctl system call */
    if (ioctl(fd, FS_IOC_FIEMAP, fiemap) < 0) {
        counter.fiemap_ioctl_failed++;
        //fprintf(stderr, "fiemap ioctl() failed\n");
        return -1;
    }

    /* Resize fiemap to allow us to read in the extents */
    size_t extents_size = sizeof(struct fiemap_extent) * fiemap->fm_mapped_extents;
    fiemap = (struct fiemap *) realloc(fiemap, sizeof(struct fiemap) + extents_size);
    if (!fiemap) {
        fprintf(stderr, "Out of memory allocating fiemap\n");	
        return -1;
    }

    memset(fiemap->fm_extents, 0, extents_size);
    fiemap->fm_extent_count = fiemap->fm_mapped_extents;
    // fiemap->fm_mapped_extents = 0; // XXX: need?

    /* 2. Finally, get each extent information */
    if (ioctl(fd, FS_IOC_FIEMAP, fiemap) < 0) {
        counter.fiemap_ioctl_failed++;
        //fprintf(stderr, "fiemap ioctl() failed\n");
        return -1;
    }
        
    for (size_t i = 0; i < fiemap->fm_mapped_extents; i++) {
        Extent *ext = new Extent(fiemap->fm_extents[i].fe_logical,
                fiemap->fm_extents[i].fe_physical,
                fiemap->fm_extents[i].fe_length);

        vec.push_back(ext);
    }

    delete fiemap;

    counter.fiemap_succ++;
    return 0;
}

int getExtents(std::string &file_path, std::vector<Extent *> &vec) {
    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd < 0) {
        counter.fiemap_file_not_found_failed++;
        //std::cerr << "Error opening the file." << std::endl;
        return -1;
    } 
     
    int ret = doFiemap(fd, vec);

    close(fd);

    return ret;
}
