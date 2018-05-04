//
// Created by nicekingwei on 18-4-29.
//

#ifndef MEMFS_BLOCK_FILE_H
#define MEMFS_BLOCK_FILE_H

#include "global.h"

/**
 * filenode
 */

struct block_node;

struct filenode {
    struct stat attr;
    pointer<block_node> block;

    void del(){
        block = null_pointer;
    }

    bool is_del(){
        return block.isnull();
    }
};


/**
 * file block
 */
struct block_file {
    size_t index = 0;
    size_t count = 0;

    const static size_t blocks_in_file = 4;

    const static size_t max_file = block_size*blocks_in_file / sizeof(struct stat) - 1;
    filenode nodes[max_file] = {0};

    block_file* init(size_t index) {
        this->index = index;
        count = 0;
        return this;
    }

    pointer<filenode> write_file(filenode& file) {
        if(count>=max_file) return null_pointer;

        nodes[count] = file;
        pointer<filenode> ret {index,OFFSET(block_file,nodes[count])};
        count++;

        return ret;
    }
};

#endif //MEMFS_BLOCK_FILE_H
