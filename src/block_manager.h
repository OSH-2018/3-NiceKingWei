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
const pointer<size_t> p_free_block_count = {VADDR(0,block_meta,nodes[0])};

const pointer<block_list<byte_t>> p_free_head = {VADDR(0,block_meta,nodes[1])};
const pointer<block_list<block_meta>> p_meta_head = {VADDR(0,block_meta,nodes[2])};
const pointer<block_list<block_string>> p_string_head = {VADDR(0,block_meta,nodes[3])};
const pointer<block_list<block_file>> p_file_head = {VADDR(0,block_meta,nodes[4])};
const pointer<block_list<block_skiplist>> p_skiplist_head = {VADDR(0,block_meta,nodes[5])};

const pointer<skipnode> p_skip_dummy[] = {
        {VADDR(0,block_meta,nodes[6])},
        {VADDR(0,block_meta,nodes[8])},
        {VADDR(0,block_meta,nodes[10])},
        {VADDR(0,block_meta,nodes[12])},
        {VADDR(0,block_meta,nodes[14])}
};

static_assert(sizeof(skipnode)==2*sizeof(block_node));


class block_manager {

    /**
     * allocate a new item
     * @throw bad_alloc
     */
    pointer<block_node> allocate_node() {

        auto p_node = p_meta_head->next;
        auto& block = *driver.get_block<block_meta>(p_node->start);

        if(block.count >= block_meta::max_nodes) throw std::bad_alloc();

        auto ret = pointer<block_node> {VADDR(block.index,block_meta,nodes[block.count])};
        block.count++;

        return ret;
    }


    /**
     * allocate a new block for meta block when necessary
     */
    inline void alloc_enough_meta(){
        auto p_node = p_meta_head->next;
        auto& block = *driver.get_block<block_meta>(p_node->start);
        if(block.count > block_meta::max_nodes/2) allocate(p_meta_head,block_meta::blocks_in_meta);
    }


    /**
     * binary split
     */
    inline std::list<size_t> bin_splist(size_t n){
        std::list<size_t> ret;
        size_t acc=1;
        while(n){
            if(n%2){
                ret.push_back(acc);
            }
            n/=2;
            acc*=2;
        }
        return ret;
    }




    /**
     * merge free blocknode
     */
    void merge_freeblocks(){

        // bubble sort
        for(auto s= p_free_head->next;!s.isnull();s=s->next){
            for(auto p=s;!p->next.isnull();p=p->next){
                auto q = p->next;
                if(q->start < p->start){
                    std::swap(p->start,q->start);
                    std::swap(p->end,q->end);
                    std::swap(p->next,q->next);
                }
            }
        }

        // merge
        for(auto p= p_free_head->next;!p->next.isnull();p=p->next){
            auto q = p->next;
            if(q->start == p->end){
                auto new_start = p->start;
                auto new_end = q->end;
                auto new_size = new_end - new_start;

                // proper block
                if(new_end%new_size == 0){
                    p->next = q->next;
                    p->start = new_start;
                    p->end = new_end;
                    q->del();
                }
            }
        }
    }

    /**
     * free blocks
     */
    template<typename T>
    void free(pointer<block_list<T>> p_head) {
        if(p_head.isnull()) return;
        
        pointer<block_node> p_tail(p_head);
        while(!p_tail->next.isnull()) p_tail=p_tail->next;

        p_tail->next = p_free_head->next;
        p_free_head->next = p_head->next;
        *p_free_block_count += p_head->count();

        pointer<block_node>(p_head)->del();

        if(random()%4==0) merge_freeblocks();
    }




    /**
     * write str
     */
    pointer<static_string> write_str(const char *str) {
        for(auto p_node = p_string_head->next;
            !p_node.isnull();
            p_node = p_node->next){
            auto b_str = driver.get_block<block_string>(p_node->start);
            auto ret = b_str->write_str(str);
            if(!ret.isnull()) return ret;
        }
        auto i_new_block = allocate(p_string_head, block_string::blocks_in_string)->next->start;
        return driver.get_block<block_string>(i_new_block)->init(i_new_block)->write_str(str);
    }



    /**
     * write file
     */
    pointer<filenode> write_file(filenode &file) {
        auto p_node = p_file_head->next;
        auto b_file = driver.get_block<block_file>(p_node->start);
        auto ret = b_file->write_file(file);
        if(ret.isnull()){
            auto i_new_block = allocate(p_file_head, block_file::blocks_in_file)->next->start;
            ret = driver.get_block<block_file>(i_new_block)->init(i_new_block)->write_file(file);
        }
        return ret;
    }


    /**
     * insert skipnode
     */
    pointer<skipnode> insert_skipnode(const skipnode &node) {
        auto p_node = p_skiplist_head->next;
        auto b_skiplist = driver.get_block<block_skiplist>(p_node->start);
        auto ret = b_skiplist->write_skip(node);
        if(ret.isnull()){
            auto i_new_block = allocate(p_skiplist_head, block_skiplist::blocks_in_skiplist)->next->start;
            ret = driver.get_block<block_skiplist>(i_new_block)->init(i_new_block)->write_skip(node);
        }
        return ret;
    }


    /**
     * create file(file and dir)
     */
    void file_new(const struct stat &file, const char *filename) {

        // write information
        auto dummy = allocate_node();
        dummy->init();

        filenode file_node = {file, pointer<block_list<byte_t>>(dummy)};

        auto p_file = write_file(file_node);
        auto p_filename = write_str(filename);

        skipnode skip_node_proto{null_pointer,null_pointer,p_filename,p_file};

        // alloc new node
        pointer<skipnode> new_nodes[MAX_DEPTH];
        long depth = 1 + random()%MAX_DEPTH;
        for(int i=0;i<depth;i++){
            new_nodes[i] = insert_skipnode(skip_node_proto);
            if(i) new_nodes[i]->down = new_nodes[i-1];
        }

        // find place for new node
        pointer<skipnode> pre_list[MAX_DEPTH];
        file_find_interval(filename,pre_list);

        // insert new node
        for(int i=0;i<depth;i++){
            new_nodes[i]->next = pre_list[i]->next;
            pre_list[i]->next = new_nodes[i];
        }
    }

public:
    void init_zero();

    skipnode file_find(const char *filename);
    void file_find_interval(const char *filename, pointer<skipnode> *pre_list);
    bool file_remove(const char *filename);
    bool file_create(const char *s);

    std::list<pointer<skipnode>> dir_list(const char* dirname);
    bool dir_create(const char *filename);


    /**
    * allocate
    * @throw bad_alloc
    */
    template<typename T>
    pointer<block_list<T>> allocate(pointer<block_list<T>> p_head,size_t total_size){

        // preprocess
        if(total_size > *p_free_block_count)
            throw std::bad_alloc();

        alloc_enough_meta();

        // build alloc list
        auto to_alloc_list = bin_splist(total_size);

        // alloc every block
        while(!to_alloc_list.empty()){

            auto size = to_alloc_list.back();
            to_alloc_list.pop_back();

            // try to allocate continuous blocks
            pointer<block_node> p_prev(p_free_head);
            bool succ = false;
            for(auto p_node = p_free_head->next; !p_node.isnull(); p_node = p_node->next){

                // allocation will succeed
                if(p_node->end - p_node->start >= size){

                    // break free block into pieces
                    while(p_node->end - p_node->start!=size){

                        auto p_new_node = allocate_node();

                        p_new_node->next = p_node->next;
                        p_node->next = p_new_node;

                        size_t l = p_node->start;
                        size_t r = p_node->end;
                        size_t m = (l+r)/2;

                        p_new_node->start = m;
                        p_new_node->end = r;
                        p_node->end = m;
                    }

                    // remove from old list
                    p_prev->next = p_node->next;

                    // add to new list
                    p_node->next = p_head->next;
                    p_head->next = p_node;

                    succ = true;
                    break;
                }

                p_prev = p_node;
            }


            // failed : split into two blocks
            if(!succ){
                to_alloc_list.push_back(size/2);
                to_alloc_list.push_back(size/2);
            }
        }

        p_head->count() += total_size;
        *p_free_block_count -= total_size;

        return p_head;
    }

};

extern block_manager manager;





#endif //MEMFS_BLOCK_MANAGER_H
