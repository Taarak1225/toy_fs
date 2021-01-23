/* Do not change! */
#define FUSE_USE_VERSION 29
#define _FILE_OFFSET_BITS 64
/******************/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <fuse/fuse.h>


#include "myfilesystem.h"


static FileSystemInfo *global_fs = NULL;

const char* remove_first_char(const char *name) {
    return (name + 1);
}


int myfuse_getattr(const char * name, struct stat * result) {
    // MODIFY THIS FUNCTION
    memset(result, 0, sizeof(struct stat));
    if (strcmp(name, "/") == 0) {
        result->st_mode = S_IFDIR;
    } else {
        name = remove_first_char(name);
        int size = file_size(name, global_fs);
        if (size == 1) {
            return -ENOENT;
        }
        result->st_size = size;
        result->st_nlink = 1;
        result->st_mode = S_IFREG;
    }
    return  0;
}

int myfuse_readdir(const char * name, void * buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * info) {
    // MODIFY THIS FUNCTION
    if (strcmp(name, "/") == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        File *file = global_fs->filelist.head->next;
        while(file != NULL) {
            filler(buf, file->dir.file_name, NULL, 0);
            file = file->next;
        }
        return 0;
    } else {
        return -ENOENT;
    }
}

int myfuse_unlink(const char *name) {
    name = remove_first_char(name);
    int retcode = delete_file(name, global_fs);
    if (retcode == 0) {
        return 0;
    } else {
        return -ENOENT;
    }
}

int myfuse_rename(const char *oldname, const char *newname) {
    oldname = remove_first_char(oldname);
    newname = remove_first_char(newname);
    File *oldfile = NULL;
    File *newfile = NULL;
    // oldname not exist
    if ((oldfile = is_exist(oldname, global_fs)) == NULL) {
        return -ENOENT;
    }
    // newname already exist
    if ((newfile = is_exist(newname, global_fs) != NULL)) {
        return -EEXIST;
    }
    int retcode = rename_file(oldname, newname, global_fs);
    if (retcode == 1) {
        return -ENOENT;
    }
    return 0;
}

int myfuse_truncate(const char *name, off_t len) {
    name = remove_first_char(name);
    int retcode = resize_file(name, len, global_fs);
    // failed
    if (retcode != 0) {
        return -ENOENT;
    }
    return 0;
}

int myfuse_open(const char *name, struct fuse_file_info *info) {
    name = remove_first_char(name);
    File *file = is_exist(name, global_fs);
    if (file == NULL) {
        return -ENOENT;
    }
    return 0;
}
int myfuse_read(const char *name, char *buffer, size_t count, off_t offset, struct fuse_file_info *info) {
    name = remove_first_char(name);
    int retcode = read_file(name, offset, count, buffer, global_fs);
    if (retcode == 1) {
        return -ENOENT;
    } else if (retcode  == 2) {
        return 0;
    } else if (retcode == 3) {
        //  verification error in the Merkle hash tree
        return -EIO;
    }
    return count;
}
int myfuse_write(const char *name, const char *buffer, size_t count, off_t offset, struct fuse_file_info *info) {
    name = remove_first_char(name);
    int retcode = write_file(name, offset, count, buffer, global_fs);
    if (retcode == 1 || retcode == 2) {
        // no file or offset greater than file  length
        return -ENOENT;
    } else if (retcode == 3) {
        // no free space
        return -ENOSPC;
    }
    return count;
}

int myfuse_release(const char *name, struct fuse_file_info *info) {
    name = remove_first_char(name);
    File *file = is_exist(name, global_fs);
    if (file == NULL) {
        return -ENOENT;
    }
    return 0;
}

void * myfuse_init(struct fuse_conn_info *info) {
    global_fs = init_fs("cmake-build-debug/before/16_file_data", "cmake-build-debug/before/16_directory_table", "cmake-build-debug/before/16_hash_data", 1);
    return NULL;
}

void myfuse_destroy(void *pVoid) {
    close_fs(global_fs);
}

int myfuse_create(const char *name, mode_t mode, struct fuse_file_info *info) {
    name = remove_first_char(name);
    File *file = is_exist(name, global_fs);
    if (file != NULL) {
        return -EEXIST;
    }
    int retcode = create_file(name, 0, global_fs);
    if (retcode == 2) {
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
    int ret = fuse_main(argc, argv, &operations, NULL);
    return ret;
}

