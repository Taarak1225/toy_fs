#ifndef RUNTEST_H
#define RUNTEST_H

#include "myfilesystem.h"

#define TEST_STRING "this is a read-write text content test string."

void print_stat(super_block *sys);

int make_empty_file(const char *filename, uint32_t size);

void make_empty_fs_file(const char *filename, int fsize, const char *dirtname, int dsize, const char *hashname, int hsize);

void remove_fs_file(const char *filename, const char *dirtname, const char *hashname);

int no_operation();

int normal_operation();

int error_operation();

int rw_operation();

#endif