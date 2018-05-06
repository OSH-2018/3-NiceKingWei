//
// Created by nicekingwei on 18-4-29.
//

#ifndef MEMFS_BLOCK_STRING_H
#define MEMFS_BLOCK_STRING_H

#include "global.h"

/**
 * static string
 */
struct static_string{
    size_t n;

    bool is_del(){
        return n==SIZE_MAX;
    }

    char* str(){
        assert(!is_del());
        return (char*)(this+1);
    }

    void del(){
        n = SIZE_MAX;
    }

};


/**
 * string block
 */
class block_string {

    size_t index = 0;       // index of this block
    size_t cur = 0;         // relative cursor


    size_t array_offset() {
        return sizeof(block_string);
    }

    byte_t* array() {
        return ((byte_t*)this) + array_offset();
    }

    size_t sizeof_safe_str(size_t n) {
        return sizeof(size_t) + n;
    }

    static size_t align(size_t x){
        return (x/4+1)*4;
    }

public:

    static const size_t blocks_in_string = 4;

    block_string* init(size_t index){
        this->index = index;
        cur = 0;
        return this;
    }

    /**
     * write string
     * @return null pointer if failed
     */
    pointer<static_string> write_str(const char* s) {
        size_t n = strlen(s);
        size_t max_cur = block_size*blocks_in_string - array_offset();
        size_t size_s = sizeof_safe_str(n);

        // can write into this block
        size_t new_cur = align(cur+size_s);

        assert(cur!=0 || new_cur<max_cur);  // cur==0 => new_cur<max_cur todo: string has max length

        if(new_cur<max_cur){
            auto start = (static_string*)(array() + cur);

            auto off = array_offset()+cur;
            pointer<static_string> ret (index+off/block_size,off%block_size);

            start->n = n;               // n
            memcpy(start->str(),s,n);   // array

            cur = new_cur;

            return ret;
        } else {
            return null_pointer;
        }
    }
};


#endif //MEMFS_BLOCK_STRING_H
