//
// Created by nicekingwei on 18-4-30.
//

#include "block_manager.h"


block_manager manager;

/**
 * init block zero
 */
void block_manager::init_zero() {

    // init
    auto b_meta = driver.get_block<block_meta>(0);

    b_meta->index = 0;
    b_meta->count = 17;

    // blocknode dummy head
    for(int i=0;i<6;i++){
        b_meta->nodes[i].init();
    }

    // whole block
    b_meta->nodes[17] = block_node {0,block_count,null_pointer};
    b_meta->nodes[0].next = pointer<block_node> {0,OFFSET(block_meta,nodes[17])};

    // alloc
    auto i_meta = allocate(p_meta_head,block_meta::blocks_in_meta)->start;
    auto i_string = allocate(p_free_head,block_string::blocks_in_string)->start;
    auto i_file = allocate(p_file_head,block_file::blocks_in_file)->start;
    auto i_skiplist = allocate(p_skiplist_head,block_skiplist::blocks_in_skiplist)->start;

    driver.get_block<block_string>(i_string)->init(i_string);
    driver.get_block<block_file>(i_file)->init(i_file);
    driver.get_block<block_skiplist>(i_file)->init(i_skiplist);

    // init skiplist
    for(int i=0;i<MAX_DEPTH;i++){
        p_skip_dummy[i]->init();
    }

    dir_create("/");
}



/**
 * allocate
 * @throw bad_alloc
 */
pointer<block_node> block_manager::allocate(pointer<block_node> p_head,size_t total_size){

    // preprocess
    if(total_size > *p_free_block_count) {
        throw std::bad_alloc();
    } else {
        // allocate a new block for meta block when necessary
        auto p_node = p_meta_head->next;
        auto& block = *driver.get_block<block_meta>(p_node->start);

        if(block.count > block_meta::max_nodes/2) allocate(p_meta_head,block_meta::blocks_in_meta);
    }


    // build alloc list
    std::list<size_t> to_alloc_list;
    size_t acc=1;
    while(total_size){
        if(total_size%2){
            to_alloc_list.push_back(acc);
        }
        total_size/=2;
        acc*=2;
    }

    // alloc every block
    while(!to_alloc_list.empty()){

        auto size = to_alloc_list.back();
        to_alloc_list.pop_back();

        // try to allocate continuous blocks
        auto p_prev = p_free_head;
        bool succ = false;
        for(auto p_node = p_free_head->next; !p_node.isnull(); p_node = p_node->next){

            // allocation will succeed
            if(p_node->end - p_node->start >= size){

                // break free block into pieces
                while(p_node->end - p_node->start!=size){

                    auto p_new_node = allocate_node();

                    p_node->next = p_new_node;

                    size_t l = p_node->start;
                    size_t r = p_node->end;
                    size_t m = (l+r)/2;

                    p_new_node->start = m;
                    p_new_node->end = r;
                    p_new_node->next = p_node->next;

                    p_node->next = p_new_node;
                    p_node->end = m;
                }

                // remove from old list
                p_prev->next = p_node->next;

                // add to new list
                p_node->next = p_head->next;
                p_head->next = p_node;

                succ = true;
                break;
            }

            p_prev = p_node;
        }


        // failed : split into two blocks
        if(!succ){
            to_alloc_list.push_back(size/2);
            to_alloc_list.push_back(size/2);
        }
    }

    p_head->count() += total_size;

    return p_head;
}



/**
 * allocate a new item
 * @throw bad_alloc
 */
pointer<block_node> block_manager::allocate_node() {
    auto p_node = p_meta_head->next;
    auto& block = *driver.get_block<block_meta>(p_node->start);

    if(block.count >= block_meta::max_nodes) throw std::bad_alloc();

    auto ret = pointer<block_node>{block.index,OFFSET(block_meta,nodes[block.count])};
    block.count++;

    return ret;
}


/**
 * free blocks
 */
void block_manager::free(pointer<block_node> p_head) {

    auto p_tail = p_head;
    while(!p_tail->next.isnull()) p_tail=p_tail->next;

    p_tail->next = p_free_head->next;
    p_free_head->next = p_head->next;
    *p_free_block_count += p_head->count();

    p_head->del();

    if(random()%4==0) merge_freeblocks();
}


/**
 * merge free blocks
 */
void block_manager::merge_freeblocks(){

    // bubble sort
    for(auto s=p_free_head->next;!s.isnull();s=s->next){
        for(auto p=s;!p->next.isnull();p=p->next){
            auto q = p->next;
            if(q->start < p->start){
                std::swap(p->start,q->start);
                std::swap(p->end,q->end);
                std::swap(p->next,q->next);
            }
        }
    }

    // merge
    for(auto p=p_free_head->next;!p->next.isnull();p=p->next){
        auto q = p->next;
        if(q->start == p->end){
            auto new_start = p->start;
            auto new_end = q->end;
            auto new_size = new_end - new_start;

            // proper block
            if(new_end%new_size == 0){
                p->next = q->next;
                p->start = new_start;
                p->end = new_end;
                q->del();
            }
        }
    }
}



/**
 * write str
 */
pointer<static_string> block_manager::write_str(const char *str) {
    for(auto p_node = p_string_head->next;
        !p_node.isnull();
        p_node = p_node->next){
        auto b_str = driver.get_block<block_string>(p_node->start);
        auto ret = b_str->write_str(str);
        if(!ret.isnull()) return ret;
    }
    auto i_new_block = allocate(p_string_head,block_string::blocks_in_string)->start;
    return driver.get_block<block_string>(i_new_block)->init(i_new_block)->write_str(str);
}



/**
 * write file
 */
pointer<filenode> block_manager::write_file(filenode &file) {
    auto p_node = p_file_head->next;
    auto b_file = driver.get_block<block_file>(p_node->start);
    auto ret = b_file->write_file(file);
    if(ret.isnull()){
        auto i_new_block = allocate(p_file_head,block_file::blocks_in_file)->start;
        ret = driver.get_block<block_file>(i_new_block)->init(i_new_block)->write_file(file);
    }
    return ret;
}


/**
 * insert skipnode
 */
pointer<skipnode> block_manager::insert_skipnode(const skipnode &node) {
    auto p_node = p_skiplist_head->next;
    auto b_skiplist = driver.get_block<block_skiplist>(p_node->start);
    auto ret = b_skiplist->write_file(node);
    if(ret.isnull()){
        auto i_new_block = allocate(p_skiplist_head,block_skiplist::blocks_in_skiplist)->start;
        ret = driver.get_block<block_skiplist>(i_new_block)->init(i_new_block)->write_file(node);
    }
    return ret;
}



#define LT(p,s) (*(p) < (s))
#define GTE(p,s) ((p).isnull() || *(p) >= (s))
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
    auto p_int_r = pointer<skipnode> null_pointer;

    // object in (int_l,int_r]
    while(true){

        if(p_int_l.isnull()) return failed;

        // shrink the interval
        while(true){
            auto try_int_l = p_int_l->next;
            if(try_int_l.isnull()) break;
            auto try_int_r = try_int_l->next;

            if(LT(try_int_l,filename) && GTE(try_int_r,filename)){
                p_int_l = try_int_l;
                p_int_r = try_int_r;
            }else{
                break;
            }
        }

        // found
        if(EQ(p_int_r,filename)){
            auto p_object = p_int_r;
            pointer<skipnode> down;
            while(true){
                down = p_object->down;
                if(down.isnull()) return *p_object;
            }
        }

        // down
        p_int_l = p_int_l->down;
        p_int_r = p_int_r->down;
    }
}


/**
 * find interval
 */
void block_manager::file_find_interval(const char *filename,pointer<skipnode> pre_list[]) {

    int d = MAX_DEPTH-1;

    // (minimal,maximal]
    auto p_int_l = p_skip_dummy[MAX_DEPTH-1];
    auto p_int_r = pointer<skipnode> null_pointer;


    // object in (int_l,int_r]
    while(true){

        if(p_int_l.isnull()) break;

        // shrink the interval
        while(true){
            auto try_int_l = p_int_l->next;
            if(try_int_l.isnull()) break;
            auto try_int_r = try_int_l->next;

            if(LT(try_int_l,filename) && GTE(try_int_r,filename)){
                p_int_l = try_int_l;
                p_int_r = try_int_r;
            }else{
                break;
            }
        }

        pre_list[d--] = p_int_l;

        // down
        p_int_l = p_int_l->down;
        p_int_r = p_int_r->down;
    }
}


/**
 * create file
 */
void block_manager::file_create(const struct stat &file, const char *filename) {

    // write information
    auto dummy = allocate_node();
    dummy->init();

    filenode file_node = {file,dummy};

    auto p_file = write_file(file_node);
    auto p_filename = write_str(filename);

    skipnode skip_node_proto{null_pointer,null_pointer,p_filename,p_file};

    // alloc new node
    pointer<skipnode> new_nodes[MAX_DEPTH];
    long depth = 1 + random()%MAX_DEPTH;
    for(int i=0;i<depth;i++){
        new_nodes[i] = insert_skipnode(skip_node_proto);
        if(i) new_nodes[i]->down = new_nodes[i-1];
    }

    // find place for new node
    pointer<skipnode> pre_list[MAX_DEPTH];
    file_find_interval(filename,pre_list);

    // insert new node
    for(int i=0;i<MAX_DEPTH;i++){
        new_nodes[i]->next = pre_list[i]->next;
        pre_list[i]->next = new_nodes[i];
    }
}


/*
 * allocate block for file
 */
void block_manager::file_alloc(pointer<skipnode> file, size_t n) {
    allocate(file->file->block,n);
}


/*
 * remove file
 * @return false if failed
 */
bool block_manager::file_remove(const char* filename) {
    pointer<skipnode> pre_list[MAX_DEPTH];
    file_find_interval(filename,pre_list);

    bool succ = false;
    for(int i=0;i<MAX_DEPTH;i++){
        pointer<skipnode> pre = pre_list[0];
        auto to_del = pre->next;
        if(EQ(to_del,filename)){
            if(!succ){
                // delete filename and filenode
                to_del->filename->del();
                free(to_del->file->block);
                to_del->file->del();
            }
            succ = true;
            // delete skipnode
            pre->next = to_del->next;
            to_del->del();
        }
    }
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

    auto prefix_s = dir.filename->str();
    auto prefix_n = dir.filename->n;


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
                if(fn_s[i]!=prefix_s[i]){
                    in_dir = false;
                    leave_dir = true;
                    break;
                }
            }
            for(int i=prefix_n;i<fn_n;i++) if(fn_s[i]=='/'){
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
void block_manager::dir_create(const char* s){
    std::string filename(s);
    if(utils::path_is_valid(filename) && utils::path_is_dir(filename)){
        struct stat new_file = {0};
        new_file.st_uid = getuid();
        new_file.st_gid = getgid();
        new_file.st_ctim = {time(nullptr),0};
        new_file.st_atim = {time(nullptr),0};
        new_file.st_mtim = {time(nullptr),0};
        new_file.st_blksize = block_size;
        new_file.st_blocks = block_count;
        new_file.st_size = new_file.st_blocks*new_file.st_blksize;
        new_file.st_mode = S_IFDIR | 0755;
        file_create(new_file,s);
    }
}



/**
 * get block address of a file
 */
byte_t * block_manager::file_block(pointer<filenode> file, size_t index, bool can_create) {

    auto meta_node = file->block;
    if(index >= meta_node->count()){
        if(can_create){
            allocate(file->block, index-meta_node->count());
            file->attr.st_blocks = index;
        } else {
            return nullptr;
        }
    }

    size_t ipos = meta_node->count()-1;
    auto ppos = file->block->next;
    while(ipos >= index){
        size_t new_ipos = ipos - (ppos->end - ppos->start);
        if(new_ipos < index){
            size_t num = ppos->start + index - (new_ipos+1);
            return driver.get_block_base(num);
        }
        ipos = new_ipos;
        ppos = ppos->next;
    }

    return nullptr;
}
