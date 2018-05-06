//
// Created by nicekingwei on 18-4-29.
//

#ifndef MEMFS_GLOBAL_H
#define MEMFS_GLOBAL_H

//#define DEBUG

#include <iostream>
#include <string>
#include <algorithm>
#include <utility>
#include <regex>
#include <ctime>
#include <list>
#include <fstream>
#include <unistd.h>
#include <fuse.h>
#include <fcntl.h>
#include <climits>
#include <cassert>
#include <cstring>

#define OFFSET(type,member) ((size_t)(&(((type*)0)->member)))
#define VADDR(index,type,member) (index)+OFFSET(type,member)/block_size,OFFSET(type,member)%block_size

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
        assert(i>=0 && i<block_count);
        return (T*)(disk+i*block_size);
    }

    byte_t* get_block_base(size_t i){
        assert(i>=0 && i<block_count);
        return (disk+i*block_size);
    }

};

extern driver_object driver;


/**
 * pointer in virtual address space
 */
template<typename T>
struct pointer {

    size_t base = SIZE_MAX;
    size_t offset = SIZE_MAX;

    pointer() noexcept = default;

    pointer(size_t base,size_t offset) noexcept {
        this->base = base;
        this->offset = offset;
    };

    template<typename R>
    explicit pointer(pointer<R> p){
        base = p.base;
        offset = p.offset;
    }

    bool isnull() const{
        return base==SIZE_MAX&&offset==SIZE_MAX;
    }

    T& operator * () const {
        assert(!isnull());
        return *((T*)(driver.get_block_base(base)+offset));
    }

    T* operator -> () const {
        assert(!isnull());
        return ((T*)(driver.get_block_base(base)+offset));
    }

    bool operator == (const pointer& x) const {
        return base==x.base && offset == x.offset;
    };

    bool operator != (const pointer& x) const {
        return base!=x.base || offset != x.offset;
    };
};
#define null_pointer {SIZE_MAX,SIZE_MAX}



struct utils{
    inline static std::string path_get_parent(const std::string& s){
        std::string ss = s;
        while(!ss.empty() && ss.back()!='/') ss.pop_back();
        if(ss!="/") ss.pop_back();
        return ss;
    }

    inline static bool path_is_valid(const std::string& s){
        const char* reg = "^(/[^/ ]*)+/?$";
        return std::regex_match(s,std::regex(reg)) && s.back()!='/';
    }
};

#endif //MEMFS_GLOBAL_H
