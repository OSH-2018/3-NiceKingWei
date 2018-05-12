# 用户态内存文件系统 memfs

* 实现了基本的文件操作和目录操作

* 支持软链接、硬链接

* 支持元信息修改

* 文件索引使用跳表 (skiplist) 

* 块分配算法与 Linux 伙伴系统类似

* 能记录日志（是文件系统服务程序的日志，不保存在文件系统镜像中）

* 编译环境：支持 c++11 的 c++ 编译器，cmake 3.10 以上

* 编译选项在 compile_config.h 中

  默认编译选项为 RELEASE 和 LOCK。块大小默认 4K，文件系统大小默认 4G。

```
#define NAIVE       // NAIVE 模式静态分配空间
#define RELEASE     // RELEASE 模式不会产生 log
#define LOCK        // LOCK 模式有全局锁
const size_t block_size = 4096;         // 4 k
const size_t block_count = 1024*256*4;  // 4 G
```

* 运行环境： `linux` 操作系统 和 `fuse 2.6` 以上

* 测试
  > mkdir cmake-build-debug
  > 
  > cd cmake-build-debug
  > 
  > cmake ../
  >
  > cd ../test/
  >
  > ./test.sh
  > 

  test1 测试文件读写
  test2 测试目录操作
  test3 从github下载本仓库并编译

  src/inline_test 文件夹下的测试代码是从 log 中分离出来的，用于在 DEBUG 模式下模拟用户行为

* 不支持读写权限管理

* 不支持用户权限管理

  ```
  
  ```