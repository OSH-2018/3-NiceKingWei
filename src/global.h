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
#include <map>
#include <ctime>
#include <list>
#include <fstream>
#include <unistd.h>
#include <fuse.h>
#include <fcntl.h>
#include <climits>
#include <cstring>
#include <sys/mman.h>
#include <execinfo.h>
#include <csignal>
#include <unistd.h>
#include "log.h"

#define OFFSET(type,member) ((size_t)(&(((type*)0)->member)))
#define VADDR(index,type,member) (index)+OFFSET(type,member)/block_size,OFFSET(type,member)%block_size

typedef uint8_t byte_t;
const size_t block_size = 4096;         // 4 k
const size_t block_count = 1024*128;    // 512 M

#define assert(expr)\
if(!(expr)){\
    logger.write("assertion failed:",__STRING(expr),__FILE__,__LINE__);\
    driver.dump();\
    exit(-1);\
}

#define assert_v(expr,...)\
if(!(expr)){\
    logger.write("assertion failed:",__STRING(expr),__FILE__,__LINE__,__VA_ARGS__);\
    driver.dump();\
    exit(-1);\
}

/**
 * dirver
 * manage and allocate physical pages
 */

class driver_object;
extern driver_object driver;
//class block_manager;
//extern block_manager manager;

struct driver_object {
#ifndef NAIVE
    std::map<size_t,byte_t*> disk;
#else
    byte_t disk[block_size*block_count];
#endif

public:

    void init(){
#ifndef NAIVE
        size_t reserved = 4;
        auto base = (byte_t*)mmap(nullptr,block_size*reserved,PROT_READ|PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
        if(base == MAP_FAILED){
            logger.write("[panic]","map failed");
            throw std::bad_alloc();
        }
        memset(base,0,block_size*reserved);
        for(size_t i=0;i<reserved;i++){
            disk[i] = base + i*block_size;
        }
#endif
    }

    byte_t* get_block_base(size_t i){
        assert_v(i>=0 && i<block_count,i);
#ifndef NAIVE
        auto ret = disk.find(i);
        if(ret == disk.end()){
            auto new_addr = (byte_t*) mmap(nullptr,block_size,PROT_READ|PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
            if(new_addr == MAP_FAILED) {
                logger.write("[panic]","map failed");
                throw std::bad_alloc();
            }
            memset(new_addr,0,block_size);
            disk[i] = new_addr;
            ret = disk.find(i);
        }
        return ret->second;
#else
        return &disk[i*block_size];
#endif
    }

    template<typename T>
    inline T* get_block(size_t i){
        return (T*)get_block_base(i);
    }

    void free(size_t n){
#ifndef NAIVE
        auto r = disk.find(n);
        if(r!=disk.end()){
            munmap(r->second,block_size);
            disk.erase(r);
        }
#endif
    }

    int dump(){
        const size_t max_call = 1024;
        void *buffer[max_call];
        int n = backtrace(buffer, max_call);
        char** strs = backtrace_symbols(buffer, n);

        if (strs != nullptr) {
            for(int i=0;i<n;i++)
                logger.write(strs[i]);
        }

#ifdef DUMP
#ifdef NAIVE
        std::ofstream fout("memfs.bin");
        for(size_t i=0;i<block_count;i++){
            fout.write((char*)&disk[i],block_count);
        }
        fout.close();
        return 0;
#endif
#endif
    }
};



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
        return base==SIZE_MAX&&offset==SIZE_MAX&&offset>=block_size;
    }

    T& operator * () const {
        assert_v(!isnull(),"null pointer");
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
        if(ss.length()>1) ss.pop_back();
        return ss;
    }

    inline static bool path_is_valid(const std::string& s){
        const char* reg = "^(/[^/ ]*)+/?$";
        return std::regex_match(s,std::regex(reg)) && s.back()!='/';
    }
};

#endif //MEMFS_GLOBAL_H
