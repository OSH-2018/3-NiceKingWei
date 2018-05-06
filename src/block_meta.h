//
// Created by nicekingwei on 18-4-29.
//

#ifndef MEMFS_BLOCK_META_H
#define MEMFS_BLOCK_META_H

#include "global.h"

/**
 * block node
 * [start,end)
 */
struct block_node {
    size_t start = 0;
    size_t end = 0;
    pointer<block_node> next;

    block_node() = default;

    block_node(size_t start,size_t end,pointer<block_node> next){
        this->start = start;
        this->end = end;
        this->next = next;
    };

    bool is_end(){
        return start==end==0;
    }

    block_node* init(){
        start = end = 0;
        next = null_pointer;
        return this;
    }

    void del(){
        start = end = 0;
        next = null_pointer;
    }

    bool is_del(){
        return start == 0 && end == 0;
    }
};


template<typename T>
struct block_list: public block_node {

    inline size_t& count(){
        return start;
    }

    T* get_block(size_t index){
        if(index<count()){
            size_t ipos = count();
            auto ppos = next;
            // [0,ipos)
            while(ipos > index){
                size_t new_ipos = ipos - (ppos->end - ppos->start);
                assert(new_ipos<ipos);
                if(new_ipos <= index){
                    size_t num = ppos->start + index - new_ipos;
                    return (T*)driver.get_block_base(num);
                }
                ipos = new_ipos;
                ppos = ppos->next;
            }
        }
        return nullptr;
    }
};

/**
 * metadata of a driver
 * first page of the driver
 * allocate blocks
 */
struct block_meta {

    static const size_t blocks_in_meta = 4;
    static const auto max_nodes = block_size*blocks_in_meta / sizeof(block_node) - 2; // 2: space for header of block meta

    // data
    size_t index = 0;
    size_t count = 0;

    block_node nodes[max_nodes];

    /**
     * init
     */
    block_meta* init(size_t index){
        this->index = index;
        count = 0;
        return this;
    }
};

#endif //MEMFS_BLOCK_META_H
