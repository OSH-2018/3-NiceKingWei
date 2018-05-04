#define FUSE_USE_VERSION 26

#include "block_manager.h"


/**
 * init
 */
void* fs_init(struct fuse_conn_info *conn) {
    manager.init_zero();
    return nullptr;
}



/**
 * get attributes of a file
 * @param stbuf return attributes of a file
 */
int fs_getattr(const char* path, struct stat* stbuf) {

    memset(stbuf, 0, sizeof(struct stat));

    auto file = manager.file_find(path).file;

    if(file.isnull()){
        return -ENOENT;
    } else {
        *stbuf = file->attr;
        return 0;
    }
}



/**
 * read dir
 * @param filler int (void *buf, const char *name,const struct stat *stbuf, off_t off)
 */
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    auto dir = manager.file_find(path);
    auto files = manager.dir_list(path);

    if(dir.file.isnull()) return -ENOENT;

    filler(buf, ".", &dir.file->attr, 0);

    auto path_parent = utils::path_get_parent(path);
    if(!path_parent.empty()){
        auto parent = manager.file_find(path_parent.c_str());
        assert(!parent.file.isnull());
        filler(buf, "..", &parent.file->attr,0);
    }

    size_t n = strlen(path);

    for(auto& file:files){
        filler(buf, file->filename->str() + n, &file->file->attr, 0);
    }

    return 0;
}



/**
 * make node
 * create file
 */
int fs_mknod(const char *path, mode_t mode, dev_t dev) {
    struct stat new_file = {0};
    new_file.st_uid = getuid();
    new_file.st_gid = getgid();
    new_file.st_ctim = {time(nullptr),0};
    new_file.st_atim = {time(nullptr),0};
    new_file.st_mtim = {time(nullptr),0};
    new_file.st_blksize = block_size;
    new_file.st_blocks = 0;
    new_file.st_size = new_file.st_blocks*new_file.st_blksize;
    new_file.st_mode = S_IFREG | 0644;
    new_file.st_nlink = 1;
    manager.file_create(new_file,path);
    return 0;
}


/**
 * open
 */
int fs_open(const char *path, struct fuse_file_info *fi) {
    if(!utils::path_is_valid(path)) return -ENOENT;
    std::string filename(path);

    auto file = manager.file_find(path).file;

    if(file.isnull()){
        return -ENOENT;
    }else{
        if(utils::path_is_dir(filename)){
            return -EACCES;
        }else{
            return 0;
        }
    }
}



/**
 * read
 */
int fs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    auto file = manager.file_find(path).file;

    if(file.isnull() || utils::path_is_dir(path)){
        return -ENOENT;
    }else{
        off_t len = file->attr.st_size;
        if(offset < len){
            // [start,end]
            size_t start_block = offset/block_size;
            size_t end_block = (offset+size)/block_size;

            auto write_buf = (byte_t*)buf;
            for(size_t i=start_block;i<=end_block;i++){
                auto pb = manager.file_block(file,i);
                size_t n;
                if(i==start_block){
                    pb += offset - start_block*block_size;
                    n = (start_block+1)*block_size - offset;
                }else if(i==end_block){
                    n = offset + size - end_block*block_size;
                }else{
                    n = block_size;
                }
                memcpy(write_buf,pb,n);
                write_buf += n;
            }
            return (int)(len-offset);
        }else{
            return 0;
        }
    }
}



/**
 * write
 */
int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    auto file = manager.file_find(path).file;
    file->attr.st_size = offset + size;
    try {
        // [start,end]
        size_t start_block = offset/block_size;
        size_t end_block = (offset+size)/block_size;

        auto read_buf = (byte_t*)buf;
        for(size_t i=start_block;i<=end_block;i++) {
            auto pb = manager.file_block(file,i,true);
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
    return (int)size;
}


/**
 * register filesystem functions
 */
struct fuse_operations memfs_operations;
#define regfun(fun) memfs_operations.fun = fs_##fun;


int main(int argc, char* argv[]) {
    regfun(init);
    regfun(getattr);
    regfun(readdir);
    regfun(mknod);
    regfun(open);
    regfun(write);
    regfun(read);
    return fuse_main(argc, argv, &memfs_operations, nullptr);
}