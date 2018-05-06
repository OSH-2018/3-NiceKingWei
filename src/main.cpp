#define FUSE_USE_VERSION 26

#include "block_manager.h"
#include "log.h"


/**
 * init
 */
static void* fs_init(struct fuse_conn_info *conn) {
    logger.write("[init]");
    manager.init_zero();
    return nullptr;
}


/**
 * get attributes of a file
 * @param stbuf return attributes of a file
 */
static int fs_getattr(const char* path, struct stat* stbuf) {

//    logger.write("[get attr]",path);

    memset(stbuf, 0, sizeof(struct stat));

    auto file = manager.file_find(path).file;

    if(file.isnull()){
//        logger.write("[get attr]","null");
        return -ENOENT;
    } else {
//        logger.write("[get attr]","found");
        *stbuf = file->attr;
        return 0;
    }
}



/**
 * read dir
 * @param filler int (void *buf, const char *name,const struct stat *stbuf, off_t off)
 */
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    logger.write("[read dir]",path);

    auto dir = manager.file_find(path).file;
    auto files = manager.dir_list(path);

    if(dir.isnull()) return -ENOENT;

    filler(buf, ".", &dir->attr, 0);

    auto path_parent = utils::path_get_parent(path);
    if(!path_parent.empty()){
        auto parent = manager.file_find(path_parent.c_str());
        assert(!parent.file.isnull());
        filler(buf, "..", &parent.file->attr,0);
    }

    size_t n = strlen(path);

    for(auto& file:files){
        const char* fn = file->filename->str() + n;
        if(*fn=='/') fn++;
        filler(buf, fn , &file->file->attr, 0);
    }

    logger.write("[read dir]","succeed");
    return 0;
}



/**
 * make node
 * create file
 */
static int fs_mknod(const char *path, mode_t mode, dev_t dev) {
    logger.write("[mknod]",path);
    try{
        if(!manager.file_create(path)) return -ENOENT;
    }catch(std::bad_alloc&){
        return -ENOSPC;
    }
    logger.write("[mknod]","succeed");
    return 0;
}


/**
 * mkdir
 */
static int fs_mkdir(const char* path, mode_t) {
    logger.write("[mkdir]", path);
    try{
        if (!manager.dir_create(path)) return -ENOENT;
    }catch(std::bad_alloc&){
        return -ENOSPC;
    }
    logger.write("[mkdir]","succeed");
    return 0;
}


/**
 * open
 */
static int fs_open(const char *path, struct fuse_file_info *fi) {

    logger.write("[open]",path);

    std::string filename(path);
    auto file = manager.file_find(path).file;

    if(file.isnull()){
        return -ENOENT;
    }else{
        if(file->is_dir()){
            return -EACCES;
        }
    }
    logger.write("[open]","succeed");
    return 0;
}



/**
 * read
 */
static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

    logger.write("[read]",path,offset,size);

    auto file = manager.file_find(path).file;

    if(file.isnull() || file->is_dir()){
        return -ENOENT;
    }else{
        off_t len = file->attr.st_size;
        if(offset < len){
            off_t max_size = len - offset;
            size = std::min(size,(size_t)max_size);

            // [start,end]
            size_t start_block = offset/block_size;
            size_t end_block = (offset+size)/block_size;

            auto write_buf = (byte_t*)buf;
            for(size_t i=start_block;i<=end_block;i++){
                auto pb = file->block->get_block(i);
                size_t n;
                if(start_block==end_block){
                    pb += offset;
                    n = size;
                }else if(i==start_block){
                    pb += offset%block_size;
                    n = block_size - offset%block_size;
                }else if(i==end_block){
                    n = (offset + size)%block_size;
                }else{
                    n = block_size;
                }
                memcpy(write_buf,pb,n);
                write_buf += n;
            }
            return (int)(len-offset);
        }
    }
    logger.write("[read]","succeed");
    return 0;
}



/**
 * write
 */
static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

    logger.write("[write]",path,offset,size);

    auto file = manager.file_find(path).file;
    if(file.isnull()) return -ENOENT;

    size_t& old_n = file->block->count();
    size_t  new_n = (offset+size)/block_size+1;

    try {
        if(old_n<new_n) manager.allocate(file->block,new_n-old_n);

        // [start,end]
        size_t start_block = offset/block_size;
        size_t end_block = (offset+size)/block_size;

        auto read_buf = (byte_t*)buf;
        for(size_t i=start_block;i<=end_block;i++) {
            auto pb = file->block->get_block(i);
            size_t n;
            if(i==start_block){
                pb += offset - start_block*block_size;
                n = (start_block+1)*block_size - offset;
            }else if(i==end_block){
                n = offset + size - end_block*block_size;
            }else{
                n = block_size;
            }
            memcpy(pb,read_buf,n);
            read_buf += n;
        }
    }catch(std::bad_alloc& e){
        return -ENOSPC;
    }
    file->attr.st_size = offset + size;
    file->attr.st_blocks = file->block->count();
    logger.write("[write]","succeed");
    return (int)size;
}



/**
 * truncate
 */
static int fs_truncate(const char *path, off_t size) {
    logger.write("[truncate]",path,size);

    auto file = manager.file_find(path).file;

    if(file.isnull())
        return -ENOENT;

    try{
        if(size > file->attr.st_size){
            size_t n = size/block_size;
            if(n > file->block->count()){
                manager.allocate(file->block,n-file->block->count());
            }
        }

        file->attr.st_size = size;
        file->attr.st_blocks = file->block->count();
    }catch(std::bad_alloc&){
        return -ENOSPC;
    }
    logger.write("[truncate]","succeed");
    return 0;
}



/**
 * unlink
 */
static int fs_unlink(const char *path) {
    logger.write("[unlink]",path);
    if(!manager.file_remove(path)) return -ENOENT;
    logger.write("[unlink]","succeed");
    return 0;
}


/**
 * rename
 */
static int fs_rename(const char * old_name, const char *new_name){
    logger.write("[rename]",old_name,new_name);
    auto file = manager.file_find(old_name).file;
    if(file.isnull() || file->is_dir()) return -ENOENT;
    try {
        auto ret = fs_mknod(new_name,0,0);
        if(ret) return ret;

        auto new_file = manager.file_find(new_name).file;
        assert(!new_file.isnull());

        new_file->attr = file->attr;
        new_file->block = file->block;

        file->block = null_pointer;
        manager.file_remove(old_name);

    }catch(std::bad_alloc&){
        return -ENOSPC;
    }
    logger.write("[rename]","succeed");
    return 0;
}


/**
 * rmdir
 */
static int fs_rmdir (const char * dirname){
    logger.write("[rmdir]",dirname);
    if(std::string(dirname) == "/") return 0;
    auto dir = manager.file_find(dirname).file;
    if(dir.isnull() || !dir->is_dir()) return -ENOENT;
    auto file = manager.dir_list(dirname);
    if(!file.empty()) return -EPERM;
    manager.file_remove(dirname);
    logger.write("[rmdir]","succeed");
    return 0;
}


/**
 * register filesystem functions
 */
struct fuse_operations memfs_operations;
#define regfun(fun) memfs_operations.fun = fs_##fun;



int main(int argc, char* argv[]) {
    regfun(init);
    regfun(getattr);
    regfun(mkdir);
    regfun(readdir);
    regfun(mknod);
    regfun(open);
    regfun(write);
    regfun(read);
    regfun(truncate);
    regfun(unlink);
    regfun(rename);
    regfun(rmdir);
    logger.write("[main]");
#ifdef DEBUG
    fs_init(nullptr);
    struct stat x;

    // test0
    char read[64];
    char buf[] = "skysissi, I love you!\n";

/*    fs_getattr("/",&x);
    fs_getattr("/.Trash",&x);
    fs_mkdir("/x",0);
    fs_mknod("/x/sky",O_RDWR,0);
    fs_mkdir("/x/y",0);
    fs_rmdir("/x/y");
    fs_mknod("/x/nice",O_RDWR,0);
    fs_unlink("/x/nice");
    fs_open("/x/sky", nullptr);
    fs_write("/x/sky",buf, sizeof(buf),0, nullptr);
    fs_rename("/x/sky","/skysissi");
    fs_read("/skysissi",read, sizeof(buf),0, nullptr);
    std::cout<<buf;
    fs_unlink("/skysissi");
    fs_getattr("/skysissi",&x);
    fs_mkdir("/x/y",0);*/

    // test2
    fs_mkdir("/dir1",0);
    fs_mkdir("/dir1/dir11",0);
    fs_mknod("/dir1/x",0,0);
    fs_write("/dir1/x",buf, sizeof(buf),0, nullptr);
    fs_read("/dir/x",read, sizeof(buf),0, nullptr);
    std::cout<<buf;
    fs_rmdir("/dir1/dir11");
    fs_unlink("/dir1/x");
    fs_rmdir("/dir1");
#else
    return fuse_main(argc, argv, &memfs_operations, nullptr);
#endif
}