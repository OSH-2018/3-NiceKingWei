cmake_minimum_required(VERSION 3.10)
project(memfs)

find_package(PkgConfig)
pkg_check_modules(FUSE REQUIRED fuse)

set(CMAKE_CXX_STANDARD 11)

add_executable(memfs src/main.cpp src/global.h src/block_file.h src/block_meta.h src/block_skiplist.h src/block_string.h src/global.cpp src/block_manager.cpp src/block_manager.h src/log.h src/log.cpp src/test_code/test1.h src/test_code/test2.h src/test_code/test3.h)

target_link_libraries(memfs fuse)
target_compile_options(memfs PUBLIC ${FUSE_CFLAGS_OTHER})
target_compile_options(memfs PUBLIC -rdynamic)