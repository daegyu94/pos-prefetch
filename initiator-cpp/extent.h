#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

#include "my_types.h"

#define DMFP_EXTENT_INFO
#ifdef DMFP_EXTENT_INFO
#define dmfp_extent_info(str, ...) \
    printf("[INFO] Extent::%s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_extent_info(str, ...) do {} while (0)
#endif

//#define DMFP_EXTENT_DEBUG
#ifdef DMFP_EXTENT_DEBUG
#define dmfp_extent_debug(str, ...) \
    printf("[DEBUG] Extent::%s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_extent_debug(str, ...) do {} while (0)
#endif

typedef struct Extent {
    uint64_t lba;
    uint64_t pba;
    uint32_t length;

    uint64_t *readahead_bitmap;
    uint8_t *ref_cnts;

    uint32_t sum_refcnf;

    Extent(uint64_t lba, uint64_t pba = -1UL, uint32_t length = 0) {
        this->lba = lba;
        this->pba = pba;
        this->length = length;

        if (pba == -1UL) { 
            readahead_bitmap = nullptr;
            ref_cnts = nullptr;
        } else {
            uint32_t num_pages = length >> PAGE_SHIFT;
            uint32_t bitmap_size = num_pages / 64 + 1;
            
            this->readahead_bitmap = new uint64_t[bitmap_size]; 
            this->ref_cnts = new uint8_t[num_pages];
            memset(this->readahead_bitmap, 0x00, sizeof(uint64_t) * bitmap_size);
            memset(this->ref_cnts, 0x00, sizeof(uint8_t) * num_pages);
        }
        sum_refcnf = 0;
        
        dmfp_extent_debug("Extent(), lba=%lu, pba=%lu, length=%u\n", 
                this->lba, this->pba, this->length);
    }

    ~Extent() {
        if (readahead_bitmap) {
            delete[] readahead_bitmap;
        }
        if (ref_cnts) {
            delete[] ref_cnts;
        }
        dmfp_extent_debug("~Extent(), delete readahead_bitmap and ref_cnts %u\n", 
                readahead_bitmap ? 1 : 0);
    }

    /* copy constructor */
    Extent(const Extent& other) {
        lba = other.lba;
        pba = other.pba;
        length = other.length;
        sum_refcnf = other.sum_refcnf;
        
        uint32_t num_pages = length >> PAGE_SHIFT;
        uint32_t bitmap_size = num_pages / 64 + 1;
        if (other.readahead_bitmap) {
            readahead_bitmap = new uint64_t[bitmap_size];
            memcpy(readahead_bitmap, other.readahead_bitmap, bitmap_size * sizeof(uint64_t));
        } else {
            readahead_bitmap = nullptr;
        }

        if (other.ref_cnts) {
            ref_cnts = new uint8_t[num_pages];
            memcpy(ref_cnts, other.ref_cnts, num_pages * sizeof(uint8_t));
        } else {
            ref_cnts = nullptr;
        }
        dmfp_extent_debug("Copy constructor num_pages=%u, bitmap_size=%u\n", 
                num_pages, bitmap_size);
    }

    uint64_t PageIndex2ExtentIndex(uint64_t index) {
        return lba > 0 ? index % (lba >> PAGE_SHIFT) : index;
    }
    void SetReadaheadBitmap(uint64_t index) {
        uint64_t i = PageIndex2ExtentIndex(index);
        readahead_bitmap[i / 64] |= (1ULL << (i % 64));
        //printf("%s: index=%lu, i=%lu, %lu\n", __func__, index, i, readahead_bitmap[i / 64]);
    }
    
    void ClearReadaheadBitmap(uint64_t index) {
        uint64_t i = PageIndex2ExtentIndex(index);
        readahead_bitmap[i / 64] &= ~(1ULL << (i % 64)); 
    }

    bool TestReadaheadBitmap(uint64_t index) {
        uint64_t i = PageIndex2ExtentIndex(index);
        return (readahead_bitmap[i / 64] & (1ULL << (i % 64)));
    }

    void AddRefCnt(uint64_t index) {
        uint64_t i = PageIndex2ExtentIndex(index);
        if (ref_cnts[i] < 255) {
            ref_cnts[i] += 1;
            ++sum_refcnf;
        }
        //printf("%s: index=%lu, i=%lu, ref_cnts=%u, sum_refcnf=%u\n", __func__, index, i, ref_cnts[i], sum_refcnf);
    }
 
    void ClearRefCnt(uint64_t index) {
        uint64_t i  = PageIndex2ExtentIndex(index);
        sum_refcnf -= ref_cnts[i];
        ref_cnts[i] = 0;
        //printf("%s: index=%lu, i=%lu, ref_cnts=%u, sum_refcnf=%u\n", __func__, index, i, ref_cnts[i], sum_refcnf);
    }

    bool IsKeyInRange(uint64_t pos) {
        return lba <= pos && pos <= (lba + length - 1);
    }

    double GetReadaheadRatio() {
        uint32_t num_pages = length >> PAGE_SHIFT;
        uint32_t bitmap_size = num_pages / 64 + 1;
        uint32_t sum = 0;
        uint32_t cnt = 0;
        
        for (int num = 0; num < bitmap_size; num++) {
            for (int i = 0; i < 64; i++) {
                uint64_t mask = 1UL << i;
                //printf("i=%d, %lu, %lu\n", i, readahead_bitmap[num], readahead_bitmap[num] & mask);
                if (readahead_bitmap[num] & mask) {
                    sum++;
                }
                if (++cnt == num_pages) {
                    goto out;
                } 
            }
        }
out:
        //printf("%s: sum=%u, num_pages=%u, cnt=%u\n", __func__, sum, num_pages, cnt);
        return 100.0 * sum / num_pages;
    }

    void Show(void) const {
        printf("lba=%lu, pba=%lu, length=%u\n", lba, pba, length);
        
        if (readahead_bitmap == nullptr || ref_cnts == nullptr) {
            return;
        }

        printf("readahead_bitmap=[] ");
        
        uint32_t bitmap_size = (length >> PAGE_SHIFT) / 64 + 1;
        for (int num = 0; num < bitmap_size; num++) {
            for (int i = 63; i >= 0; i--) {
                uint64_t mask = 1UL << i;
                //printf("i=%d, mask=%lu\n", i, mask);
                printf("%u", (readahead_bitmap[num] & mask) ? 1 : 0);

                if (i % 8 == 0 && i != 0) {
                    printf(" "); // Add a space every 8 bits for better readability
                }
            }
        }
        printf("\n");
        printf("ref_cnts=[] ");
        for (int i = 0; i < length >> PAGE_SHIFT; i++) {
            printf("%u", ref_cnts[i]);
            if (i % 8 == 0 && i != 0) {
                printf(" "); // Add a space every 8 bits for better readability
            }
        }
        printf("\n");
        printf("sum_refcnf=%u\n", sum_refcnf);
        printf("\n");
    }


    void Show(uint64_t ino, int extent_number, std::ofstream &outfile) {
        double avg_refcnt, readahead_ratio;

        if (readahead_bitmap == nullptr || ref_cnts == nullptr) {
            avg_refcnt = 0.0;
            readahead_ratio = 0.0;
        } else {
            avg_refcnt = 1.0 * sum_refcnf / (length >> 12);
            readahead_ratio = GetReadaheadRatio();
        }
       
        outfile << "ino=" << ino << ", ext=" << extent_number
            << ", lba=" << lba << ", pba=" << pba << ", length=" << length 
            << ", avg_refcnt=" << avg_refcnt 
            << ", readahead_ratio(%)=" << readahead_ratio
            << std::endl;

#if 0 
        outfile << "readahead_bitmap=[]" << std::endl;
        uint32_t num_pages = length >> PAGE_SHIFT;
        uint32_t bitmap_size = num_pages / 64 + 1;
        for (int num = 0; num < bitmap_size; num++) {
            for (int i = 63; i >= 0; i--) {
                uint64_t mask = 1UL << i;
                uint32_t value = (readahead_bitmap[num] & mask) ? 1 : 0;
                outfile << value;

                if (i % 8 == 0 && i != 0) {
                    outfile << " "; // Add a space every 8 bits for better readability
                }
            }
        }
        outfile << std::endl;
        outfile << "ref_cnts=[]" << std::endl;
        for (int i = 0; i < num_pages; i++) {
            uint32_t value = ref_cnts[i];
            outfile << value;
            if (i % 8 == 0 && i != 0) {
                outfile << " "; // Add a space every 8 bits for better readability
            }
        }
        outfile << std::endl;
#endif
    }
} Extent_t;

#if 0
static inline int extent_compare(const void *a, const void *b, void *udata) {
    const Extent_t *ua = a;
    const Extent_t *ub = b;
    int cmp;

    if (ua->lba > ub->lba) {
        cmp = 1;
    } else if (ua->lba < ub->lba) {
        cmp = -1;
    } else {
        cmp = 0;
    }

    return cmp;
}
#else
static inline int extent_compare(const void *a, const void *b, void *udata) {
    const Extent_t *key = (const Extent_t *) a;
    const Extent_t *item = (const Extent_t *) *(uint64_t *) b;
    int cmp;
    
    //printf("key= %lu %lu %u\n", key->lba, key->pba, key->length);
    //printf("item= %lu %lu %u\n", item->lba, item->pba, item->length);
    if (item->lba <= key->lba && item->lba + item->length - 1 >= key->lba) {
        cmp = 0;
    } else if (key->lba > item->lba) {
        cmp = 1;
    } else {
        cmp = -1;
    }
    return cmp;
}
#endif


static inline bool extent_iter(const void *a, void *udata) {
    Extent_t *ext = (Extent_t *) *(uint64_t *) a;
    std::vector<Extent *> *vec = reinterpret_cast<std::vector<Extent *> *>(udata);
    //ext->Show();
    vec->push_back(ext);
    return true;
}


/* TODO: */
static inline 
std::vector<std::pair<uint64_t, int>> evaluateExtentReadahead(Extent &extent, uint64_t index) {

}

static inline
std::vector<std::pair<uint64_t, int>> evaluateExtentRefCnt(Extent &extent, uint64_t index) {

}

/* evaluate readahead and avg ref cnts */
static inline std::vector<std::pair<uint64_t, uint32_t>>
evaluateExtent(Extent &extent, uint64_t start_index, bool cond1, bool cond2) 
{
    std::vector<std::pair<uint64_t, uint32_t>> vec;


    return vec;
}


