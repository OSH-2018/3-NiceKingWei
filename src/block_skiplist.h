//
// Created by nicekingwei on 18-4-29.
//

#ifndef MEMFS_BLOCK_SKIPLIST_H
#define MEMFS_BLOCK_SKIPLIST_H

#include <cmath>
#include "global.h"
#include "block_string.h"
#include "block_file.h"

/**
 * skiplist
 */
#define MAX_DEPTH 8

struct skipnode {
    pointer<skipnode> next;
    pointer<skipnode> down;
    pointer<static_string> filename;
    pointer<filenode> file;

    skipnode* init(){
        down = next = null_pointer;
        file = null_pointer;
        filename = null_pointer;
        return this;
    }

    bool operator < (const char *str_s2) const noexcept {
        if(filename.isnull()) return true;  // null is minimal

        static_string& s1 = *filename;
        size_t len_s1 = s1.n;
        const char* str_s1 = s1.str();
        for(int i=0;i<len_s1;i++){
            if(str_s1[i]<str_s2[i]) return true;
            else if(str_s1[i]>str_s2[i]) return false;
        }
        return str_s2[len_s1]!=0;
    }

    bool operator==(const char *str_s2) const noexcept {
        if(filename.isnull()) return false;  // null is minimal

        static_string& s1 = *filename;
        size_t len_s1 = s1.n;
        const char* str_s1 = s1.str();
        for(int i=0;i<len_s1;i++){
            if(str_s1[i]!=str_s2[i]) return false;
        }
        return str_s2[len_s1]=='\0';
    }

    bool operator>=(const char *str_s2) const noexcept {
        return !((*this)<str_s2);
    }

    void del(){
        next = down = null_pointer;
        filename = null_pointer;
        file = null_pointer;
    }

    bool is_del(){
        return filename.isnull() && file.isnull();
    }
};



/**
 * skiplist block
 */
struct block_skiplist {

    size_t index = 0;
    size_t count = 0;

    const static size_t blocks_in_skiplist = 1;

    const static size_t max_skip = block_size*blocks_in_skiplist / sizeof(skipnode) - 1;
    skipnode nodes[max_skip];

    block_skiplist* init(size_t index) {
        this->index = index;
        count = 0;
        return this;
    }

    pointer<skipnode> write_skip(const skipnode &node) {
        if(count>=max_skip) return null_pointer;

        nodes[count] = node;
        pointer<skipnode> ret {VADDR(index,block_skiplist,nodes[count])};
        count++;

        return ret;
    }

    static size_t rand_depth(){
        auto h = (size_t)log2((byte_t)random());
        if(h>=MAX_DEPTH || h==0) return rand_depth();
        return h;
    }

};

static_assert(sizeof(block_skiplist)<block_size*block_skiplist::blocks_in_skiplist,"block size error");


#endif //MEMFS_BLOCK_SKIPLIST_H
