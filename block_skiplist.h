//
// Created by nicekingwei on 18-4-29.
//

#ifndef MEMFS_BLOCK_SKIPLIST_H
#define MEMFS_BLOCK_SKIPLIST_H

#include "global.h"

/**
 * skiplist block
 */
struct block_skiplist {

    size_t index = 0;
    size_t count = 0;

    const static size_t blocks_in_skiplist = 4;

    const static size_t max_file = block_size*blocks_in_skiplist / sizeof(struct stat) - 1;
    skipnode nodes[max_file];

    block_skiplist* init(size_t index) {
        this->index = index;
        count = 0;
        return this;
    }

    pointer<skipnode> write_file(const skipnode& node) {
        if(count>=max_file) return null_pointer;

        nodes[count] = node;
        pointer<skipnode> ret {index,OFFSET(block_skiplist,nodes[count])};
        count++;

        return ret;
    }
};


#endif //MEMFS_BLOCK_SKIPLIST_H
