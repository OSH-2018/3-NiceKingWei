//
// Created by nicekingwei on 18-4-29.
//

#ifndef MEMFS_GLOBAL_H
#define MEMFS_GLOBAL_H

#include <iostream>
#include <string>
#include <algorithm>
#include <utility>
#include <regex>
#include <ctime>
#include <list>
#include <unistd.h>
#include <fuse.h>
#include <fcntl.h>
#include <cassert>
#include <cstring>

#define OFFSET(type,member) ((size_t)(&((type*)0)->member))

typedef uint8_t byte_t;
const size_t block_size = 4096;         // 4 k
const size_t block_count = 1024*128;    // 512 M



/**
 * dirver
 * manage and allocate physical pages
 * todo: allocate when needed
 */
class driver_object {

    byte_t disk[block_size*block_count] = {0};

public:

    template<typename T>
    inline T* get_block(size_t i){
        assert(i<block_count);
        return (T*)(disk+i*block_size);
    }

    byte_t* get_block_base(size_t i){
        assert(i<block_count);
        return (disk+i*block_size);
    }

};

extern driver_object driver;


/**
 * pointer in virtual address space
 */
template<typename T>
struct pointer {

    size_t base = 0;
    size_t offset = 0;

    pointer() noexcept = default;

    pointer(size_t base,size_t offset) noexcept {
        this->base = base;
        this->offset = offset;
    };

    T& operator * () const {
        return *((T*)driver.get_block_base(base)+offset);
    }

    T* operator -> () const {
        return ((T*)driver.get_block_base(base)+offset);
    }

    bool isnull() const{
        return base==0&&offset==0;
    }

    bool operator == (const pointer& x) const {
        return base==x.base && offset == x.offset;
    };

    bool operator != (const pointer& x) const {
        return base!=x.base || offset != x.offset;
    };
};
#define null_pointer {0,0}

/**
 * skiplist
 */
#define MAX_DEPTH 4

struct filenode;
struct static_string;

struct skipnode {
    pointer<skipnode> next;
    pointer<skipnode> down;
    pointer<static_string> filename;
    pointer<filenode> file;

    skipnode* init(){
        down = next = null_pointer;
        return this;
    }

    bool operator < (const char* str) const;
    bool operator == (const char* str) const;
    bool operator >= (const char* str) const;

    void del(){
        next = down = null_pointer;
        filename = null_pointer;
        file = null_pointer;
    }

    bool is_del(){
        return filename.isnull() && file.isnull();
    }
};

struct utils{
    inline static std::string path_get_parent(const std::string& s){
        std::string ss = s;
        while(!ss.empty() && ss.back()!='/') ss.pop_back();
        return ss;
    }

    inline static bool path_is_valid(const std::string& s){
        const char* reg = "^(/[^/ ]*)+/?$";
        return std::regex_match(s,std::regex(reg));
    }

    inline static bool path_is_dir(const std::string& s){
        return s.back()=='/';
    }
};

#endif //MEMFS_GLOBAL_H
