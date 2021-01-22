/* Do not change! */
#define FUSE_USE_VERSION 29
#define _FILE_OFFSET_BITS 64
/******************/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fuse.h>
#include <errno.h>

#include "myfilesystem.h"

char * file_data_file_name = NULL;
char * directory_table_file_name = NULL;
char * hash_data_file_name = NULL;

super_block *helper;

int myfuse_getattr(const char * path, struct stat * result) {
    char realname[strlen(path)];
    memcpy(realname, path + 1, strlen(realname));

    memset(result, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        result->st_mode = S_IFDIR;
    }
    else {
        result->st_mode = S_IFREG;
        int size = file_size((char *)realname, helper);

        if (size >= 0) {
            result->st_size = size;
        }
        else {
            return -ENOENT;
        }
    }
    return 0;
}

int myfuse_readdir(const char * path, void * buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * fi) {
    if (strcmp(path, "/") == 0) {
        int dt = open(directory_table_file_name, O_RDONLY);
        char filename[72];
        while (read(dt, filename, 72) > 0) {
            if (strlen(filename)) {
                filler(buf, filename, NULL, 0);
            }
        }
        close(dt);
        return 0;
    }
    return -ENOENT;
}

int myfuse_unlink(const char * path) {
    char realname[strlen(path)];
    memcpy(realname, path + 1, strlen(realname));

    int result = delete_file((char *)realname, helper);
    if (result == 0) {
        return 0;
    }
    return -ENOENT;
}

int myfuse_rename(const char * oldname, const char * newname) {
    char realname[strlen(oldname)];
    memcpy(realname, oldname + 1, strlen(oldname));
    char newrealname[strlen(newname)];
    memcpy(newrealname, newname + 1, strlen(newname));

    int result = rename_file((char *)realname, (char *)newrealname, helper);
    if (result == 1) {
        return -ENOENT;
    }
    return 0;
}

int myfuse_truncate(const char * path, off_t length) {
    char realname[strlen(path)];
    memcpy(realname, path + 1, strlen(path));

    int result = resize_file((char *)realname, length, helper);
    if (result == 1) {
        return -ENOENT;
    }
    if (result == 2) {
        return -ENOSPC;
    }
    return 0;
}

int myfuse_open(const char * path, struct fuse_file_info *ffi) {
    char realname[strlen(path)];
    memcpy(realname, path + 1, strlen(path));

    int dt = open(directory_table_file_name, O_RDONLY);
    char buf[72];
    while (read(dt, buf, 72) > 0) {
        if (strcmp(buf, realname) == 0) {
            ffi->fh = 1;
            return 0;
        }
    }
    return -ENOENT;
}

int myfuse_read(const char * path, char * buf, size_t count, off_t offset, struct fuse_file_info * ffi) {
    char realname[strlen(path)];
    memcpy(realname, path + 1, strlen(path));

    size_t length = file_size(realname, helper);
    if (length > count) {
        length = count;
    }
    int result = read_file((char *)realname, offset, length, (char *)buf, helper);
    printf("read_file() return: %d\n\n", result);
    if (result == 0) {
        return length;
    }
    if (result == 1) {
        return -ENONET;
    }
    if (result == 2) {
        return -ENOSPC;
    }
    return 0;
}

int myfuse_write(const char * path, const char * buf, size_t count, off_t offset, struct fuse_file_info * ffi) {
    char realname[strlen(path)];
    memcpy(realname, path + 1, strlen(path));

    int result = write_file((char *)realname, offset, count, (char *)buf, helper);
    if (result == 0) {
        return count;
    }
    if (result == 1) {
        return -ENOENT;
    }
    return 0;
}

int myfuse_release(const char * path, struct fuse_file_info * ffi) {
    if (ffi->fh == 1) {
        ffi->fh = 0;
        return 0;
    }
    return -ENFILE;
}

void * myfuse_init(struct fuse_conn_info * fci) {
    helper = init_fs(file_data_file_name, directory_table_file_name, hash_data_file_name, 1);
    return helper;
}

void myfuse_destroy(void * helper) {
    close_fs(helper);
    return;
}

int myfuse_create(const char * path, mode_t mode, struct fuse_file_info * ffi) {
    int result = create_file(path, 0, helper);

    if (result == 1) {
        return -EEXIST;
    }
    else if (result == 2) {
        return -ENOSPC;
    }
    return 0;
}

struct fuse_operations operations = {
    .getattr = myfuse_getattr,
    .readdir = myfuse_readdir,
    .unlink = myfuse_unlink,
    .rename = myfuse_rename,
    .truncate = myfuse_truncate,
    .open = myfuse_open,
    .read = myfuse_read,
    .write = myfuse_write,
    .release = myfuse_release,
    .init = myfuse_init,
    .destroy = myfuse_destroy,
    .create = myfuse_create
};

int main(int argc, char * argv[]) {
    // MODIFY (OPTIONAL)
    if (argc >= 5) {
        if (strcmp(argv[argc-4], "--files") == 0) {
            file_data_file_name = argv[argc-3];
            directory_table_file_name = argv[argc-2];
            hash_data_file_name = argv[argc-1];
            argc -= 4;
        }
    }
    
    // After this point, you have access to file_data_file_name, directory_table_file_name and hash_data_file_name
    int ret = fuse_main(argc, argv, &operations, NULL);
    return ret;
}