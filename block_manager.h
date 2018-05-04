//
// Created by nicekingwei on 18-4-30.
//

#ifndef MEMFS_BLOCK_MANAGER_H
#define MEMFS_BLOCK_MANAGER_H

#include "global.h"
#include "block_string.h"
#include "block_file.h"
#include "block_meta.h"
#include "block_skiplist.h"


// allocation
const pointer<size_t> p_free_block_count = {0,OFFSET(block_meta,nodes[0])};

const pointer<block_node> p_free_head = {0,OFFSET(block_meta,nodes[1])};
const pointer<block_node> p_meta_head = {0,OFFSET(block_meta,nodes[2])};
const pointer<block_node> p_string_head = {0,OFFSET(block_meta,nodes[3])};
const pointer<block_node> p_file_head = {0,OFFSET(block_meta,nodes[4])};
const pointer<block_node> p_skiplist_head = {0,OFFSET(block_meta,nodes[5])};

const pointer<skipnode> p_skip_dummy[] = {
        {0,OFFSET(block_meta,nodes[6])},
        {0,OFFSET(block_meta,nodes[8])},
        {0,OFFSET(block_meta,nodes[10])},
        {0,OFFSET(block_meta,nodes[12])},
        {0,OFFSET(block_meta,nodes[14])}
};


class block_manager {

    /**
     * allocate
     * @throw bad_alloc
     */
    pointer<block_node> allocate(pointer<block_node> p_head,size_t total_size);


    /**
     * allocate a new item
     * @throw bad_alloc
     */
    pointer<block_node> allocate_node();


    /**
     * free
     */
    void free(pointer<block_node> p_head);


    /**
     * merge free blocknode
     */
    void merge_freeblocks();



    /**
     * write string
     */
    pointer<static_string> write_str(const char* str);


    /**
     * write file
     */
    pointer<filenode> write_file(filenode& file);


    /**
     * insert skipnode
     */
    pointer<skipnode> insert_skipnode(const skipnode& node);


public:
    void init_zero();

    void file_create(const struct stat &file, const char *filename);
    skipnode file_find(const char *filename);
    void file_find_interval(const char *filename, pointer<skipnode> *pre_list);
    void file_alloc(pointer<skipnode> file, size_t n);
    bool file_remove(const char *filename);
    byte_t * file_block(pointer<filenode> file, size_t index, bool can_create = false);

    std::list<pointer<skipnode>> dir_list(const char* dirname);
    void dir_create(const char *filename);
};

extern block_manager manager;





#endif //MEMFS_BLOCK_MANAGER_H
