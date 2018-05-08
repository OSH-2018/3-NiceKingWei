struct stat x;
char read[64];
char buf[] = "skysissi, I love you!\n";
char nothing[block_size+1];
fs_init(0);
fs_mkdir("/dir1",0);
fs_unlink("/dir2");
fs_mkdir("/dir1/dir11",0);
fs_mknod("/dir1/x",0,0);
fs_write("/dir1/x",buf, sizeof(buf),0, nullptr);
fs_read("/dir/x",read, sizeof(buf),0, nullptr);
std::cout<<buf;
fs_rmdir("/dir1/dir11");
fs_rename("/dir1/x","/dir1/y");
fs_mknod("/dir1/x",0,0);
fs_unlink("/dir1/y");
fs_rmdir("/dir1");
fs_mkdir("/.git",0);
fs_mkdir("/.git/obj",0);
fs_mkdir("/.git/obj/pack",0);
fs_mkdir("/.git/obj/pack/s",0);
fs_mkdir("/.git/obj/pack/s/x",0);
fs_mkdir("/.git/obj/pack/s/x/uu",0);
fs_mkdir("/.git/obj/pack/s/x/uu/y",0);
fs_mknod("/.git/obj/pack/s/x/uu/y/bugfile",0,0);
for(int i=0;i<256;i++){
fs_write("/.git/obj/pack/s/x/uu/y/bugfile",nothing,block_size-1,i*block_size, nullptr);
fs_write("/.git/obj/pack/s/x/uu/y/bugfile",nothing,1,i*block_size+block_size-1, nullptr);
}