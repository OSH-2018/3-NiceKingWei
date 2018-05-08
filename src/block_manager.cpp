//
// Created by nicekingwei on 18-4-30.
//

#include "block_manager.h"
#include "log.h"

block_manager manager;



/**
 * init block zero
 */
void block_manager::init_zero() {

    driver.init();

    // init
    auto b_meta = driver.get_block<block_meta>(0);

    b_meta->index = 0;
    b_meta->count = 24;

    // blocknode dummy head
    for(int i=1;i<7;i++){
        b_meta->nodes[i].init();
    }

    // whole block
    b_meta->nodes[22] = block_node {0,block_count,null_pointer};
    p_free_head->next = pointer<block_node> {VADDR(0,block_meta,nodes[22])};

    b_meta->nodes[23] = block_node {0,block_meta::blocks_in_meta,null_pointer};
    p_meta_head->next = pointer<block_node> {VADDR(0,block_meta,nodes[23])};


    logger.write("[init zero]","alloc");

    // alloc
    auto i_meta = allocate(p_meta_head, block_meta::blocks_in_meta)->next->start;
    p_meta_head->next = p_meta_head->next->next;
    auto i_string = allocate(p_string_head, block_string::blocks_in_string)->next->start;
    auto i_file = allocate(p_file_head, block_file::blocks_in_file)->next->start;
    auto i_skiplist = allocate(p_skiplist_head, block_skiplist::blocks_in_skiplist)->next->start;

    *p_free_block_count = block_count - block_meta::blocks_in_meta -  block_string::blocks_in_string -  block_file::blocks_in_file - block_skiplist::blocks_in_skiplist;

    logger.write("[init zero]",b_meta->index,i_string,i_file,i_skiplist);

    driver.get_block<block_string>(i_string)->init(i_string);
    driver.get_block<block_file>(i_file)->init(i_file);
    driver.get_block<block_skiplist>(i_skiplist)->init(i_skiplist);

    // init skiplist
    for(int i=0;i<MAX_DEPTH;i++){
        p_skip_dummy[i]->init();
        if(i) p_skip_dummy[i]->down = p_skip_dummy[i-1];
    }

    dir_create("/");
}




#define LT(p,s) (!(p).isnull() && *(p) < (s))
#define EQ(p,s) (!(p).isnull() && *(p) == (s))


/**
 * find file
 * null filename pointer is minimal
 * null pointer is maximal
 * @return skipnode on the bottom; null pointer in fileds if failed
 */
skipnode block_manager::file_find(const char *filename) {

    auto failed = skipnode {null_pointer,null_pointer,null_pointer,null_pointer};

    // (minimal,maximal]
    auto p_int_l = p_skip_dummy[MAX_DEPTH-1];
    auto p_int_r = p_int_l->next;

    // object in (int_l,int_r]
    while(true){

        if(p_int_l.isnull()) return failed;

        p_int_r = p_int_l->next;

        // shrink the interval
        while(LT(p_int_r,filename)){
            p_int_l = p_int_r;
            assert(!p_int_l.isnull());
            p_int_r = p_int_l->next;
        }

        // found
        if(EQ(p_int_r,filename)){
            auto p_object = p_int_r;
            while(true){
                if(p_object->down.isnull()) return *p_object;
                else p_object = p_object->down;
            }
        }

        // down
        p_int_l = p_int_l->down;
    }
}


/**
 * find interval
 */
void block_manager::file_find_interval(const char *filename,pointer<skipnode> pre_list[]) {

    int d = MAX_DEPTH-1;

    // (minimal,maximal]
    auto p_int_l = p_skip_dummy[MAX_DEPTH-1];
    auto p_int_r = p_int_l->next;

    // object in (int_l,int_r]
    while(true){

        if(p_int_l.isnull()) break;

        p_int_r = p_int_l->next;

        // shrink the interval
        while(LT(p_int_r,filename)){
            auto try_int_l = p_int_r;
            if(try_int_l.isnull()) break;
            auto try_int_r = try_int_l->next;
            p_int_l = try_int_l;
            p_int_r = try_int_r;
        }

        pre_list[d--] = p_int_l;

        // down
        p_int_l = p_int_l->down;
    }
}



/*
 * remove file
 * @return false if failed
 */
bool block_manager::file_remove(const char* filename) {

#ifdef VERBOSE
    logger.write("skiplist",dump_skip());
#endif

    if(std::string(filename)=="/") return true;
    pointer<skipnode> pre_list[MAX_DEPTH];
    file_find_interval(filename,pre_list);

    bool succ = false;
    pointer<static_string> to_del_filename = null_pointer;
    for(auto& pre : pre_list) {
        auto to_del = pre->next;
        if(EQ(to_del,filename)){
            if(!succ){
                // delete filename and filenode
                to_del_filename = to_del->filename;
                if(to_del->file->is_dir() || --to_del->file->attr.st_nlink == 0){
                    free(to_del->file->block);
                    to_del->file->del();
                }
                succ = true;
            }
            // delete skipnode
            pre->next = to_del->next;
            to_del->del();
        }
    }
    if(!to_del_filename.isnull()) to_del_filename->del();

    return succ;
}


/**
 * list files in directory
 */
std::list<pointer<skipnode>> block_manager::dir_list(const char *dirname) {

    std::list<pointer<skipnode>> ret;

    auto dir = file_find(dirname);

    if(dir.file.isnull())
        return ret;

    // generate prefix
    std::string prefix = dir.filename->str();
    while (!prefix.empty() && prefix.back()=='/') prefix.pop_back();
    prefix.push_back('/');
    size_t prefix_n = prefix.size();

    for(auto file=dir.next;!file.isnull();file=file->next){
        auto fn_s = file->filename->str();
        auto fn_n = file->filename->n;
        bool in_dir = true;
        bool leave_dir = false;

        if(fn_n <= prefix_n){
            in_dir = false;
            leave_dir = true;
        }else{
            for(int i=0;i<prefix_n;i++){
                if(fn_s[i]!=prefix[i]){
                    in_dir = false;
                    leave_dir = true;
                    break;
                }
            }

            for(size_t i=prefix_n;i<fn_n;i++) if(fn_s[i]=='/'){
                in_dir = false;
                break;
            }
        }

        if(leave_dir){
            break;
        } else if(in_dir){
            ret.push_back(file);
        }
    }
    return ret;
}



/**
 * create dir
 */
bool block_manager::dir_create(const char* s){

    std::string filename(s);

    // filter
    if(filename!="/"){
        if(!utils::path_is_valid(filename)) return false;
        auto s_parent = utils::path_get_parent(filename);
        auto f_parent = manager.file_find(s_parent.c_str()).file;
        if(f_parent.isnull()) return false;
    }


    struct stat new_file = {0};
    new_file.st_uid = getuid();
    new_file.st_gid = getgid();
    new_file.st_ctim = {time(nullptr),0};
    new_file.st_atim = {time(nullptr),0};
    new_file.st_mtim = {time(nullptr),0};
    new_file.st_blksize = block_size;
    if(filename=="/"){
        new_file.st_blocks = block_count;
    }else{
        new_file.st_blksize = 0;
    }
    new_file.st_size = new_file.st_blocks*new_file.st_blksize;
    new_file.st_nlink = 2;
    new_file.st_mode = S_IFDIR | 0755;
    file_new(new_file,filename.c_str());

    return true;
}


/**
 * create file
 */
bool block_manager::file_create(const char* s){


    std::string filename(s);

    // filter
    if(!utils::path_is_valid(filename)) return false;
    auto s_parent = utils::path_get_parent(filename);
    auto f_parent = manager.file_find(s_parent.c_str()).file;
    if(f_parent.isnull()) return false;

    struct stat new_file = {0};
    new_file.st_uid = getuid();
    new_file.st_gid = getgid();
    new_file.st_ctim = {time(nullptr),0};
    new_file.st_atim = {time(nullptr),0};
    new_file.st_mtim = {time(nullptr),0};
    new_file.st_blksize = block_size;
    new_file.st_blocks = 0;
    new_file.st_size = 0;
    new_file.st_nlink = 1;
    new_file.st_mode = S_IFREG | 0664;
    file_new(new_file,s);

    return true;
}

std::string block_manager::dump_skip() {
    std::stringstream ret;
    ret<<"\n";
    for(int i=MAX_DEPTH-1;i>=0;i--){
        auto node = p_skip_dummy[i]->next;
        while(!node.isnull()) {
            ret<<node->filename->str()<<" ";
            node = node->next;
        }
        ret<<"\n";
    }
    return ret.str();
}

#define output_node(node)\
ret << "("<< node->start << "," << node->end << ") ";

/*
for(size_t i=node->start;i<node->end;i++){\
    assert(binmap.find(i)==binmap.end());\
    binmap[i] = true;\
}
*/

std::string block_manager::dump_alloc() {
    std::map<int,bool> binmap;
    std::stringstream ret;
    ret<<*p_free_block_count<<"\n";
    ret<<"free:";
    for(auto node = p_free_head->next;!node.isnull();node=node->next){
        output_node(node);
    }
    ret<<"\n";
    ret<<"skip:";
    for(auto node = p_skiplist_head->next;!node.isnull();node=node->next){
        output_node(node);
    }
    ret<<"\n";
    ret<<"meta:";
    for(auto node = p_meta_head->next;!node.isnull();node=node->next){
        output_node(node);
    }
    ret<<"\n";
    ret<<"string:";
    for(auto node = p_string_head->next;!node.isnull();node=node->next){
        output_node(node);
    }
    ret<<"\n";
    ret<<"file:";
    for(auto node = p_file_head->next;!node.isnull();node=node->next){
        output_node(node);
    }
    ret<<"\n";
    for(auto file = p_skip_dummy[0]->next;!file.isnull();file=file->next){
        ret<<file->filename->str()<<":";
        for(auto node = file->file->block->next;!node.isnull();node=node->next){
            output_node(node);
        }
        ret<<"\n";
    }
    return ret.str();
}

bool block_manager::file_hardlink(const char *dest, const char *linkname) {

    auto filenode = manager.file_find(dest);
    if(filenode.file.isnull() || filenode.file->is_dir()) return false;

    filenode.file->attr.st_nlink++;

    auto proto = filenode;
    proto.next = proto.down = null_pointer;
    proto.filename = write_str(linkname);

    file_new(proto);

    return true;
}
