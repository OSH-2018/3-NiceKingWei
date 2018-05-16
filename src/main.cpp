#define FUSE_USE_VERSION 26

#include "block_manager.h"
#include "log.h"
#include "lock.h"


/**
 * init
 */
static void* fs_init(struct fuse_conn_info *conn) {

    calllog(0);
    logger.write("[fun]","fs_init()");

    manager.init_zero();
    return nullptr;
}


/**
 * get attributes of a file
 * @param stbuf return attributes of a file
 */
static int fs_getattr(const char* path, struct stat* stbuf) {

    calllog(path,0);

#ifdef DEBUG
    struct stat s;
    stbuf = &s;
#endif

    memset(stbuf, 0, sizeof(struct stat));

    auto file = manager.file_find(path).file;

    if(file.isnull()){
#ifdef VERBOSE
        logger.write("[getattr]","failed");
#endif
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
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    calllog(path,"buf","filler",offset,0);

    logger.write("[read dir]",path);

    auto dir = manager.file_find(path).file;
    auto files = manager.dir_list(path);

    if(dir.isnull()) {
        logger.write("[read dir]","failed");
        return -ENOENT;
    }

    filler(buf, ".", &dir->attr, 0);

    std::string path_parent;
    utils::path_get_parent(path,path_parent);
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

    return 0;
}



/**
 * make node
 * create file
 */
static int fs_mknod(const char *path, mode_t mode, dev_t dev) {
    calllog(path,0,0);
    logger.write("[mknod]",path);
    try{
        if(!manager.file_create(path)){
            logger.write("[mknod]","failed");
            return -ENOENT;
        }
#ifdef VERBOSE
        logger.write("[mknod]","succeed");
#endif
    }catch(std::bad_alloc&){
        logger.write("[mknod]","failed");
        return -ENOSPC;
    }
    return 0;
}


/**
 * mkdir
 */
static int fs_mkdir(const char* path, mode_t) {
    calllog(path,0);
    logger.write("[mkdir]", path);
    try{
        if (!manager.dir_create(path)){
            logger.write("[mkdir]","failed");
            return -ENOENT;
        }
    }catch(std::bad_alloc&){
        logger.write("[mkdir]","failed");
        return -ENOSPC;
    }
    return 0;
}


/**
 * open
 */
static int fs_open(const char *path, struct fuse_file_info *fi) {
    calllog(path,0);

    logger.write("[open]",path);

    std::string filename(path);
    auto file = manager.file_find(path).file;

    if(file.isnull()){
        logger.write("[open]","failed");
        return -ENOENT;
    }else{
        if(file->is_dir()){
            logger.write("[open]","failed");
            return -EACCES;
        }
    }
    return 0;
}



/**
 * read
 */
static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

    calllog(path,"buf",size,offset,0);

    logger.write("[read]",path,offset,size);

    auto file = manager.file_find(path).file;

    if(file.isnull() || file->is_dir()){
        logger.write("[read]","failed");
        return -ENOENT;
    }else{
        if(size==0) return 0;

        off_t len = file->attr.st_size;
        if(offset < len){
            off_t max_size = len - offset;
            if(size>max_size){
                memset(buf+max_size,0,size-max_size);
                size = (size_t)max_size;
            }
            // [start,end]
            size_t start_block = offset/block_size;
            size_t end_block = (offset+size-1)/block_size;

            auto write_buf = (byte_t*)buf;
            for(size_t i=start_block;i<=end_block;i++){

                auto pb = file->block->get_block(i);

                size_t start_byte=0,end_byte=block_size;
                if(i==start_block) start_byte = offset%block_size;
                if(i==end_block) end_byte = (offset+size)%block_size;
                if(end_byte==0) end_byte=block_size;
#ifdef NAIVE
                logger.write("[read]",(pb - driver.disk),end_byte-start_byte);
#endif
                memcpy(write_buf,pb+start_byte,end_byte-start_byte);
                write_buf += end_byte-start_byte;

            }
            return (int)size;
        }
    }
    return 0;
}


size_t rich_alloc(size_t old_n,size_t new_n){
    // new_n > old_n
    return std::max(new_n,old_n+old_n/8);
}

/**
 * write
 */
static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

    calllog(path,"buf",size,offset,0);

    logger.write("[write]",path,offset,size);

    auto file = manager.file_find(path).file;
    if(file.isnull()) {
        logger.write("[write]","failed");
        return -ENOENT;
    }

    if(size==0) return 0;

    size_t& old_n = file->block->count();
    size_t  new_n = (offset+size-1)/block_size+1;

    try {

        if(old_n<new_n){
            manager.allocate(file->block,rich_alloc(old_n,new_n));
#ifdef VERBOSE
            logger.write("[write]","blocks: ",file->block->count());
#endif
        }

        // [start,end]
        size_t start_block = offset/block_size;
        size_t end_block = (offset+size-1)/block_size;

        auto read_buf = (byte_t*)buf;
        for(size_t i=start_block;i<=end_block;i++) {
            auto pb = file->block->get_block(i);

            size_t start_byte=0,end_byte=block_size;
            if(i==start_block) start_byte = offset%block_size;
            if(i==end_block) end_byte = (offset+size)%block_size;
            if(end_byte==0) end_byte = block_size;
#ifdef NAIVE
            logger.write("[write]",(pb - driver.disk),end_byte-start_byte);
#endif
            memcpy(pb+start_byte,read_buf,end_byte-start_byte);
            read_buf += end_byte-start_byte;
        }
    }catch(std::bad_alloc& e){
        logger.write("[write]","failed");
        return -ENOSPC;
    }
    file->attr.st_size = std::max((size_t)file->attr.st_size,offset + size);
    file->attr.st_blocks = file->block->count();
    return (int)size;
}



/**
 * truncate
 */
static int fs_truncate(const char *path, off_t size) {

    calllog(path,size);

    logger.write("[truncate]",path,size);

    auto file = manager.file_find(path).file;

    if(file.isnull()){
        logger.write("[truncate]","failed");
        return -ENOENT;
    }

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
        logger.write("[truncate]","failed");
        return -ENOSPC;
    }
    return 0;
}



/**
 * unlink
 */
static int fs_unlink(const char *path) {

    calllog(path);

    if(!manager.file_remove(path)){
        logger.write("[unlink]","failed");
        return -ENOENT;
    }
    return 0;
}


/**
 * link
 */
static int fs_link(const char *dest, const char *linkname){

    calllog(dest,linkname);

    try {
        manager.file_hardlink(dest,linkname);
    }catch(std::bad_alloc&){
        logger.write("[link]","failed");
        return -ENOSPC;
    }

    return 0;
}


/**
 * rename
 */
static int fs_rename(const char * old_name, const char *new_name){

    calllog(old_name,new_name);

    global_mtx.unlock();

    manager.file_remove(new_name);

    if(auto r1 = fs_link(old_name,new_name)) {
        logger.write("[rename]","failed");
        return r1;
    }

    if(auto r2 = fs_unlink(old_name)) {
        logger.write("[rename]","failed");
        return r2;
    }

    return 0;
}


/**
 * rmdir
 */
static int fs_rmdir (const char * dirname){

    calllog(dirname);

    logger.write("[rmdir]",dirname);
    if(std::string(dirname) == "/") return 0;
    auto dir = manager.file_find(dirname).file;
    if(dir.isnull() || !dir->is_dir()) {
        logger.write("[rmdir]","failed");
        return -ENOENT;
    }
    auto file = manager.dir_list(dirname);
    if(!file.empty()){
        logger.write("[rmdir]","failed");
        return -EPERM;
    }
    manager.file_remove(dirname);
    return 0;
}


/**
 * chmod
 */
static int fs_chmod (const char * fname, mode_t mode){

    calllog(fname,mode);

    auto file = manager.file_find(fname).file;
    if(file.isnull()) return -ENOENT;
    file->attr.st_mode = mode;

    return 0;
}


/**
 * chown
 */
static int fs_chown(const char * fname, uid_t uid, gid_t gid){

    calllog(fname,uid,gid);

    auto file = manager.file_find(fname).file;
    if(file.isnull()) return -ENOENT;
    file->attr.st_gid = gid;
    file->attr.st_uid = uid;

    return 0;
}


/**
 * readlink
 */
static int fs_readlink(const char *path, char *buf, size_t size){
    calllog(path,"ret",size);
    auto file = manager.file_find(path).file;

    if(file.isnull()){
        logger.write("[readlink]","failed");
        return -ENOENT;
    }

    if(file->is_symlink()){
        logger.write("[readlink]","failed");
        return -EACCES;
    }

    try{
        // read
        off_t max_size = file->attr.st_size;
        if(size>max_size){
            memset(buf+max_size,0,size-max_size);
            size = (size_t)max_size;
        }
        // [start,end]
        size_t start_block = 0;
        size_t end_block = (size-1)/block_size;

        auto write_buf = (byte_t*)buf;
        for(size_t i=start_block;i<=end_block;i++){
            auto pb = file->block->get_block(i);
            size_t n;
            if(start_block==end_block){
                n = size;
            }else if(i==start_block){
                n = block_size;
            }else if(i==end_block){
                n = size%block_size;
            }else{
                n = block_size;
            }
            memcpy(write_buf,pb,n);
            write_buf += n;
        }
    }catch(std::bad_alloc&){
        logger.write("[readlink]","failed");
        return -ENOSPC;
    }

    return 0;
}


/**
 * symlink
 */
static int fs_symlink(const char *dest, const char *symname){

    calllog(dest,symname);

    global_mtx.unlock();
    auto r1 = fs_mknod(symname,0,0);
    if(r1) {
        logger.write("[symlink]","failed");
        return r1;
    }

    auto file = manager.file_find(symname).file;
    assert(!file.isnull());
    file->mk_symlink();

    auto r2 = fs_write(symname,dest,strlen(dest),0, nullptr);
    if(r2<0) {
        logger.write("[symlink]","failed");
        return r2;
    }


    return 0;
}


/**
 * utimens
 */
static int fs_utimens(const char *path, const struct timespec tv[2]){

    calllog(path,"tv");

    auto file = manager.file_find(path).file;
    if(file.isnull()) return -ENOENT;
    file->attr.st_atim = tv[0];
    file->attr.st_mtim = tv[1];

    return 0;
}


void signal_handle(int sig_num)
{
    if(sig_num == SIGSEGV) {
        assert(0);
    } else if(sig_num == SIGABRT){
        assert(0);
    }
}

#ifdef DEBUG
int filler(void *buf, const char *name,const struct stat *stbuf, off_t off){
    return 0;
}
#endif

/**
 * register filesystem functions
 */
struct fuse_operations memfs_operations;
#define regfun(fun) memfs_operations.fun = fs_##fun;

int main(int argc, char* argv[]) {

    signal(SIGSEGV,signal_handle);

#ifdef DEBUG
    #include "test_code/test7.h"
    return 0;
#else
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
    regfun(chmod);
    regfun(chown);
    regfun(symlink);
    regfun(readlink);
    regfun(link);
    regfun(utimens);
    logger.write("[main]");
    return fuse_main(argc, argv, &memfs_operations, nullptr);
#endif
}