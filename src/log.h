//
// Created by nicekingwei on 18-5-4.
//

#ifndef MEMFS_LOG_H
#define MEMFS_LOG_H

#include "global.h"

struct log{
    std::ofstream fout;

    log(){
        fout.open("memfs.log");
    }

    void write(){
#ifdef DEBUG
        std::cout<<std::endl;
        std::cout.flush();
#endif
        fout<<std::endl;
        fout.flush();
    }

    template<typename T,typename...TS>
    void write(T x,TS... args){
#ifdef DEBUG
        std::cout<<x<<' ';
#endif
        fout<<x<<' ';
        write(args...);
    }
};

extern log logger;

#endif //MEMFS_LOG_H
