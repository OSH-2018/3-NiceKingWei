struct stat x;
char read[64];
char buf[] = "skysissi, I love you!\n";
char nothing[block_size+1];
fs_init(0);
memset(nothing,0xff,block_size);
fs_getattr("/",&x);
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
fs_mkdir("/x/y",0);