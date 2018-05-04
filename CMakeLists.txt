cmake_minimum_required(VERSION 3.10)
project(memfs)

find_package(PkgConfig)
pkg_check_modules(FUSE REQUIRED fuse)

set(CMAKE_CXX_STANDARD 11)

add_executable(memfs main.cpp global.h block_file.h block_meta.h block_skiplist.h block_string.h  global.cpp block_manager.cpp block_manager.h)

target_link_libraries(memfs fuse)
target_compile_options(memfs PUBLIC ${FUSE_CFLAGS_OTHER})