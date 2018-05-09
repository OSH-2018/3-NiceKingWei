//
// Created by nicekingwei on 18-5-8.
//

#ifndef MEMFS_COMPILE_CONFIG_H
#define MEMFS_COMPILE_CONFIG_H

#include <unistd.h>

//#define NAIVE
#define RELEASE
#define LOCK
const size_t block_size = 4096;         // 4 k
const size_t block_count = 1024*256*4;  // 4 G

#endif //MEMFS_COMPILE_CONFIG_H
