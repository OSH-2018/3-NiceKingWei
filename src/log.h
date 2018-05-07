//
// Created by nicekingwei on 18-5-4.
//

#ifndef MEMFS_LOG_H
#define MEMFS_LOG_H

#include <fstream>
#include <iostream>
#include <mutex>

struct log{
    std::ofstream fout;
    std::mutex mtx;

    log(){
        fout.open("memfs.log");
    }

    int write_(){
#ifdef DEBUG
        std::cout<<std::endl;
        std::cout.flush();
#endif
        fout<<std::endl;
        fout.flush();
        return 0;
    }

    template<typename T,typename...TS>
    int write_(T x,TS... args){
#ifdef DEBUG
        std::cout<<x<<' ';
#endif
        fout<<x<<' ';
        return write_(args...);
    }

    template<typename...TS>
    int write(TS... args){
        mtx.lock();
        write_(args...);
        mtx.unlock();
        return 0;
    };

    template<typename T>
    int write_fun_(T x){
        fout<<x;
        fout.flush();
        return 0;
    }

    int write_fun_(const char* x){
        fout<<'"'<<x<<'"';
        fout.flush();
        return 0;
    }

    template<typename T,typename Q,typename...TS>
    int write_fun_(T x,Q y,TS... args){
        fout<<x<<',';
        return write_fun_(y,args...);
    }

    template<typename Q,typename...TS>
    int write_fun_(const char* x,Q y,TS... args){
        fout<<'"'<<x<<"\",";
        return write_fun_(y,args...);
    }

    template<typename...TS>
    int write_fun(const char* funname,TS...arg){
        mtx.lock();
        fout<<"[call]"<<funname<<"(";
        write_fun_(arg...);
        fout<<");\n";
        mtx.unlock();
        return 0;
    }
};

extern log logger;

#define calllog(...) logger.write_fun(__FUNCTION__,__VA_ARGS__)

#endif //MEMFS_LOG_H
