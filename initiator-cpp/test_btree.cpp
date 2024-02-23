#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "btree.h"
#include "extent.h"

using namespace std;

void test() {
    size_t max_items_per_node = 2; // 0 is 256
    int num_extents = 200;

    struct btree *btree = btree_new(sizeof(Extent *), max_items_per_node, 
            extent_compare, NULL);
    
    cout << sizeof(Extent) << " " << sizeof(Extent *) << endl;

    for (int i = 0; i < num_extents; i++) {
        uint64_t lba = i + 10;
        uint64_t pba = i + 10;
        uint32_t length = 128;
    
        Extent *extent = new Extent(lba, pba, length); 
        cout << "item's pointer= " << extent << ", " << &extent << endl;
        btree_set(btree, &extent);
    }

    for (int i = 0; i < num_extents; i++) {
        uint64_t lba = i + 10;

        Extent extent(lba); 
        Extent *ret_extent;

        //ret_extent = (Extent *) btree_delete(btree, &extent);
        ret_extent = (Extent *) *(uint64_t *) btree_get(btree, &extent);
        if (ret_extent) {
            cout << "find: " << ret_extent->lba << " " << ret_extent->pba << endl;
        }
    }
    
    btree_free(btree);
}

int main() {
    test();
    return 0;
}
