//
// Created by nicekingwei on 18-5-4.
//

#ifndef MEMFS_LOG_H
#define MEMFS_LOG_H

#define DEBUG
#define NAIVE
//#define DUMP
//#define VERBOSE

#include <fstream>
#include <iostream>
#include <mutex>

struct log{
    std::ofstream fout;
    std::mutex mtx;

    log(){
        fout.open("memfs.log");
    }

    int write_(std::ostream& out){
        out<<std::endl;
        out.flush();
        return 0;
    }

    template<typename T,typename...TS>
    int write_(std::ostream& out,T x,TS... args){
        out<<x<<' ';
        return write_(out,args...);
    }

    template<typename...TS>
    int write(TS... args){
        mtx.lock();
#ifdef DEBUG
        write_(std::cout,args...);
#else
        write_(fout,args...);
#endif
        mtx.unlock();
        return 0;
    };

    template<typename T>
    int write_fun_(std::ostream& out,T x){
        out<<x;
        out.flush();
        return 0;
    }

    int write_fun_(std::ostream& out,const char* x){
        out<<'"'<<x<<'"';
        out.flush();
        return 0;
    }

    template<typename T,typename Q,typename...TS>
    int write_fun_(std::ostream& out,T x,Q y,TS... args){
        out<<x<<',';
        return write_fun_(out,y,args...);
    }

    template<typename Q,typename...TS>
    int write_fun_(std::ostream& out,const char* x,Q y,TS... args){
        out<<'"'<<x<<"\",";
        return write_fun_(out,y,args...);
    }

    template<typename...TS>
    int write_fun(const char* funname,TS...arg){
        mtx.lock();
#ifdef DEBUG
        std::cout<<"[call] "<<funname<<"(";
        write_fun_(std::cout,arg...);
        std::cout<<");\n";
#else
        fout<<"[call] "<<funname<<"(";
        write_fun_(fout,arg...);
        fout<<");\n";
        mtx.unlock();
#endif
        return 0;
    }
};

extern log logger;

#define calllog(...) logger.write_fun(__FUNCTION__,__VA_ARGS__)

#endif //MEMFS_LOG_H
