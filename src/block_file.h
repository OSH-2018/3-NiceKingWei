//
// Created by nicekingwei on 18-4-29.
//

#ifndef MEMFS_BLOCK_FILE_H
#define MEMFS_BLOCK_FILE_H

#include "global.h"
#include "block_meta.h"

/**
 * filenode
 */

struct filenode {
    struct stat attr;
    pointer<block_list<byte_t>> block;

    void del(){
        block = null_pointer;
    }

    bool is_del(){
        return block.isnull();
    }

    bool is_dir(){
        return (attr.st_mode & S_IFDIR)!=0;
    }

    bool is_symlink(){
        return (attr.st_mode & S_IFLNK) == 0;
    }

    void mk_symlink(){
        attr.st_mode = S_IFLNK | 0777;
    }
};


/**
 * file block
 */
struct block_file {
    size_t index = 0;
    size_t count = 0;

    const static size_t blocks_in_file = 1;

    const static size_t max_file = block_size*blocks_in_file / sizeof(filenode) - 1;
    filenode nodes[max_file];

    block_file* init(size_t index) {
        this->index = index;
        count = 0;
        return this;
    }

    pointer<filenode> write_file(filenode& file) {
        if(count>=max_file) return null_pointer;

        nodes[count] = file;
        pointer<filenode> ret {VADDR(index,block_file,nodes[count])};
        count++;

        return ret;
    }

};

static_assert(sizeof(block_file)<block_file::blocks_in_file*block_size,"block size error");

#endif //MEMFS_BLOCK_FILE_H
