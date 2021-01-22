#include "runtest.h"

#define FILE_NAME "./COMP2017temp/file.bin"
#define DIRT_NAME "./COMP2017temp/dirt.bin"
#define HASH_NAME "./COMP2017temp/hash.bin"

#define AFTER_FILE "./COMP2017temp/after_file.bin"
#define AFTER_DIRT "./COMP2017temp/after_dirt.bin"
#define AFTER_HASH "./COMP2017temp/after_hash.bin"

void print_stat(super_block *sys) {
    printf("------------------------------------------------------\n");
    print_node(sys->used_list);
    print_dirt(sys);
    printf("------------------------------------------------------\n");

    return;
}

int make_empty_file(const char *filename, uint32_t size) {
    int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);

    char *buf = (char *)calloc(size, 1);
    write(fd, buf, size);
    lseek(fd, 0, SEEK_SET);

    return fd;
}

void make_empty_fs_file(const char *filename, int fsize, const char *dirtname, int dsize, const char *hashname, int hsize) {
    int file_fd, dirt_fd, hash_fd;

    file_fd = make_empty_file(FILE_NAME, fsize);
    dirt_fd = make_empty_file(DIRT_NAME, dsize);
    hash_fd = make_empty_file(HASH_NAME, hsize);

    close(file_fd);
    close(dirt_fd);
    close(hash_fd);
}

void remove_fs_file(const char *filename, const char *dirtname, const char *hashname) {
    if (unlink(filename) != 0) {
        printf("remove_fs_file failed in unlink: %s\n", filename);
        return;
    }
    if (unlink(dirtname) != 0) {
        printf("remove_fs_file failed in unlink: %s\n", dirtname);
        return;
    }
    if (unlink(hashname) != 0) {
        printf("remove_fs_file failed in unlink: %s\n", hashname);
        return;
    }
}

int no_operation() {
    make_empty_fs_file(FILE_NAME, 1024, DIRT_NAME, 288, HASH_NAME, 112);

    super_block *sys = (super_block *)init_fs(FILE_NAME, DIRT_NAME, HASH_NAME, 1);
    if (sys == NULL) {
        printf("no_operation failed in init_fs\n");
        return -1;
    }

    printf("file_data: %d, block_num: %d, ", sys->file_size, sys->block_num);
    printf("dirt_data: %d, dirt_num: %d, ", sys->dirt_size, sys->dirt_num);
    printf("hash_data: %d, hash_num: %d\n", sys->hash_size, sys->hash_num);

    print_stat(sys);
    
    close_fs(sys);
    remove_fs_file(FILE_NAME, DIRT_NAME, HASH_NAME);
    return 0;
}

int normal_operation() {
    make_empty_fs_file(FILE_NAME, 1024, DIRT_NAME, 288, HASH_NAME, 112);

    super_block *sys = (super_block *)init_fs(FILE_NAME, DIRT_NAME, HASH_NAME, 1);
    if (sys == NULL) {
        printf("no_operation failed in init_fs\n");
        return -1;
    }

    printf("file_data: %d, block_num: %d, ", sys->file_size, sys->block_num);
    printf("dirt_data: %d, dirt_num: %d, ", sys->dirt_size, sys->dirt_num);
    printf("hash_data: %d, hash_num: %d\n", sys->hash_size, sys->hash_num);

    create_file("first", 500, sys);
    create_file("second", 500, sys);
    print_stat(sys);

    resize_file("first", 476, sys);
    print_stat(sys);

    create_file("third", 40, sys);
    rename_file("first", "FIRST", sys);
    rename_file("second", "SECOND", sys);
    print_stat(sys);

    printf("file_size(\"third\") = %d\n", (int)file_size("third", sys));

    delete_file("SECOND", sys);
    print_stat(sys);

    create_file("newFile", 100, sys);
    create_file("anotherFile", 100, sys);
    print_stat(sys);
    
    close_fs(sys);
    remove_fs_file(FILE_NAME, DIRT_NAME, HASH_NAME);
    return 0;
}

int error_operation() {
    make_empty_fs_file(FILE_NAME, 1024, DIRT_NAME, 288, HASH_NAME, 112);

    super_block *sys = (super_block *)init_fs(FILE_NAME, DIRT_NAME, HASH_NAME, 1);
    if (sys == NULL) {
        printf("no_operation failed in init_fs\n");
        return -1;
    }

    printf("file_data: %d, block_num: %d, ", sys->file_size, sys->block_num);
    printf("dirt_data: %d, dirt_num: %d, ", sys->dirt_size, sys->dirt_num);
    printf("hash_data: %d, hash_num: %d\n", sys->hash_size, sys->hash_num);

    create_file("A", 300, sys);
    print_stat(sys);

    printf("create_file(\"test\", 2000, sys) = %d\n", create_file("test", 2000, sys));
    printf("create_file(\"A\", 500, sys) = %d\n", create_file("A", 500, sys));

    create_file("B", 200, sys);
    create_file("C", 200, sys);
    create_file("D", 200, sys);
    print_stat(sys);

    printf("create_file(\"E\", 5, sys) = %d\n", create_file("E", 5, sys));
    printf("resize_file(\"A\", 500, sys) = %d\n", resize_file("A", 500, sys));
    printf("resize_file(\"A\", -500, sys) = %d\n", resize_file("A", -500, sys));

    printf("delete_file(\"E\", sys) = %d\n", delete_file("E", sys));
    printf("rename_file(\"E\", \"X\", sys) = %d\n", rename_file("E", "X", sys));
    printf("rename_file(\"A\", \"B\", sys) = %d\n", rename_file("A", "B", sys));

    close_fs(sys);
    remove_fs_file(FILE_NAME, DIRT_NAME, HASH_NAME);
    return 0;
}

int rw_operation() {
    make_empty_fs_file(FILE_NAME, 1024, DIRT_NAME, 288, HASH_NAME, 112);

    super_block *sys = (super_block *)init_fs(FILE_NAME, DIRT_NAME, HASH_NAME, 1);
    if (sys == NULL) {
        printf("no_operation failed in init_fs\n");
        return -1;
    }

    printf("file_data: %d, block_num: %d, ", sys->file_size, sys->block_num);
    printf("dirt_data: %d, dirt_num: %d, ", sys->dirt_size, sys->dirt_num);
    printf("hash_data: %d, hash_num: %d\n", sys->hash_size, sys->hash_num);

    int res = -1;
    char *rd_buf = malloc(100);
    char *wr_buf = malloc(100);
    memcpy(wr_buf, TEST_STRING, strlen(TEST_STRING));
    wr_buf[strlen(TEST_STRING)] = '\0';
    printf("wr_buf: %s\n", wr_buf);

    create_file("A", 500, sys);
    print_stat(sys);

    memset(rd_buf, 0, strlen(rd_buf));
    res = write_file("A", 0, 99, wr_buf, sys);
    printf("write_file(\"A\", 0, 99, wr_buf, sys) = %d\n", res);
    if (res == 0) {
        read_file("A", 0, 99, rd_buf, sys);
        printf("rd_buf: %s\n", rd_buf);
    }

    create_file("B", 500, sys);
    resize_file("A", 450, sys);
    print_stat(sys);

    memset(rd_buf, 0, strlen(rd_buf));
    res = write_file("A", 440, 99, wr_buf, sys);
    printf("write_file(\"A\", 440, 99, wr_buf, sys) = %d\n", res);
    if (res == 0) {
        read_file("A", 440, 99, rd_buf, sys);
        printf("rd_buf: %s\n", rd_buf);
    }
    
    memset(rd_buf, 0, strlen(rd_buf));
    res = write_file("A", 440, 60, wr_buf, sys);
    printf("write_file(\"A\", 440, 60, wr_buf, sys) = %d\n", res);
    if (res == 0) {
        read_file("A", 440, 60, rd_buf, sys);
        printf("rd_buf: %s\n", rd_buf);
    }

    close_fs(sys);

    // save fs_file to test myfuse.c
    // remove_fs_file(FILE_NAME, DIRT_NAME, HASH_NAME);
    return 0;
}

int main() {
    // no_operation test
    printf("no_operation test\n\n");
    no_operation();

    // normal_operation test
    printf("normal_operation test\n\n");
    normal_operation();

    // error_operation test
    printf("error_operation test\n\n");
    error_operation();

    // rw_operation test
    printf("rw_operation test\n");
    rw_operation();

    return 0;
}