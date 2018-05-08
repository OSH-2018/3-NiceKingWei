//
// Created by nicekingwei on 18-5-8.
//

#ifndef MEMFS_LOCK_H
#define MEMFS_LOCK_H

#include <mutex>

struct fake_lock{
    void lock(){}
    void unlock(){}
};

#ifndef LOCK
using my_lock_t = fake_lock;
#else
using my_lock_t = std::mutex;
#endif

extern my_lock_t global_mtx;

struct global_lock{

    global_lock(){
        global_mtx.lock();
    }

    virtual ~global_lock(){
        global_mtx.unlock();
    }
};



#endif //MEMFS_LOCK_H
