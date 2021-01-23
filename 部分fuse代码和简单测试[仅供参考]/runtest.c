#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "myfilesystem.h"

#define TEST(x) test(x, #x)

/* You are free to modify any part of this file. The only requirement is that when it is run, all your tests are automatically executed */

/* Some example unit test functions */

int compare_two_file_binary(char *name1, char *expect, char *filetype) {
    FILE *fp1 = fopen(name1, "rb+");
    FILE  *fp2 = fopen(expect, "rb+");
    if (fp1 == NULL  || fp2 == NULL){
        printf("open file failed\n");
        return 1;
    }
    fseek(fp1, 0, SEEK_END);
    int size = ftell(fp1);
    fseek(fp1, 0 , SEEK_SET);
    char *buf1 = (char*)malloc(size);
    char *buf2 = (char*)malloc(size);
    if (buf1 == NULL || buf2 == NULL) {
        printf("malloc failed\n");
        free(buf1);
        free(buf2);
        return 1;
    }
    fread(buf1, size, 1, fp1);
    fread(buf2, size, 1, fp2);
    fclose(fp1);
    fclose(fp2);
    for(int i = 0; i < size; i++) {
        if(buf1[i] !=buf2[i]) {
            printf("Your output file %s differs from expected output %s\n", name1, expect);
            printf("At byte offset %d, %s contains byte value %d (decimal) but %s contains byte value %d (decimal)\n",
                   i, name1, buf1[i], expect, buf2[i]);
            printf("%s output incorrect\n", filetype);
            free(buf1);
            free(buf2);
            return 1;
        }
    }
    free(buf1);
    free(buf2);
    return 0;
}


int success() {
    return 0;
}

int failure() {
    return 1;
}

int no_operation() {
    void * helper = init_fs("before/00_file_data", "before/00_directory_table", "before/00_hash_data", 4);
    close_fs(helper);
    return 0;
}

int milestone_no_operation() {
    void * helper = init_fs("before/01_file_data", "before/01_directory_table", "before/01_hash_data", 4);
    if (helper == NULL) {
        return 1;
    }
    close_fs(helper);
    return 0;
}

int milestone_create_file() {
    void * helper = init_fs("before/02_file_data", "before/02_directory_table", "before/02_hash_data", 4);
    int ret = create_file("myfile", 500, helper);
    close_fs(helper);
    if (ret != 0) {
        return 1;
    }
    if (compare_two_file_binary("before/02_directory_table", "after/02_directory_table", "directory_table") != 0) {
        return 1;
    }
    return 0;
}

int milestone_create_file_exists() {
    void * helper = init_fs("before/03_file_data", "before/03_directory_table", "before/03_hash_data", 4);
    int ret = create_file("document.txt", 1, helper);
    close_fs(helper);
    if (ret != 1) {
        return 1;
    }
    if (compare_two_file_binary("before/03_directory_table", "after/03_directory_table", "directory_table") != 0) {
        return 1;
    }
    return 0;
}

int milestone_resize_file() {
    void * helper = init_fs("before/04_file_data", "before/04_directory_table", "before/04_hash_data", 4);
    int ret = resize_file("file1.txt", 18, helper);
    close_fs(helper);
    if (ret != 0) {
        return 1;
    }
    if (compare_two_file_binary("before/04_directory_table", "after/04_directory_table", "directory_table") != 0) {
        return 1;
    }
    return 0;
}

int milestone_resize_file_no_space() {
    void * helper = init_fs("before/05_file_data", "before/05_directory_table", "before/05_hash_data", 4);
    int ret = resize_file("assignment.pdf", 1025, helper);
    close_fs(helper);
    if (ret != 2) {
        return 1;
    }
    helper = init_fs("before/05_file_data", "before/05_directory_table", "before/05_hash_data", 4);
    ret = resize_file("assignment.pdf", 1024, helper);
    close_fs(helper);
    if (ret != 0) {
        return 1;
    }
    if (compare_two_file_binary("before/05_directory_table", "after/05_directory_table", "directtory_table") != 0) {
        return 1;
    }
    return 0;
}

int milestone_delete_file() {
    void * helper = init_fs("before/06_file_data", "before/06_directory_table", "before/06_hash_data", 4);
    int ret = delete_file("wow.file", helper);
    close_fs(helper);
    if (ret != 0) {
        return 1;
    }
    if (compare_two_file_binary("before/06_directory_table", "after/06_directory_table", "directory_table") != 0) {
        return 1;
    }
    return 0;
}

int milestone_delete_file_not_found() {
    void * helper = init_fs("before/07_file_data", "before/07_directory_table", "before/07_hash_data", 4);
    int ret = delete_file("nonexistent", helper);
    close_fs(helper);
    if (ret != 1) {
        return 1;
    }
    if (compare_two_file_binary("before/07_directory_table", "after/07_directory_table", "directory_table") != 0) {
        return 1;
    }
    return 0;
}

int milestone_rename_file() {
    void *helper = init_fs("before/08_file_data", "before/08_directory_table", "before/08_hash_data", 4);
    int ret = rename_file("test.o", "different", helper);
    close_fs(helper);
    if (ret != 0) {
        return 1;
    }
    if (compare_two_file_binary("before/08_directory_table", "before/08_directory_table", "directory_table") != 0) {
        return 1;
    }
    return 0;
}


int milestone_rename_file_exists() {
    void * helper = init_fs("before/09_file_data", "before/09_directory_table", "before/09_hash_data", 4);
    int ret = rename_file("test.o", "test.c", helper);
    close_fs(helper);
    if (ret != 1) {
        return 1;
    }
    if (compare_two_file_binary("before/09_directory_table", "after/09_directory_table", "directory_table") != 0) {
        return 1;
    }
    return 0;
}

int milestone_read_file() {
    void * helper = init_fs("before/10_file_data", "before/10_directory_table", "before/10_hash_data", 4);
    char buf[11];
    int ret = read_file("file1.txt", 5, 50, buf, helper);
    if (ret != 2) {
        close_fs(helper);
        return 1;
    }
    ret = read_file("file1.txt", 5, 10, buf, helper);
    close_fs(helper);
    if (ret != 0 || memcmp(buf, "x assisted", 10) != 0) {
        return 1;
    }
    return 0;
}

int milestone_write_file() {
    void * helper = init_fs("before/12_file_data", "before/12_directory_table", "before/12_hash_data", 4);
    char buf[] = "tests";
    int ret = write_file("file1.txt", 160, 50, buf, helper);
    if (ret != 2) {
        close_fs(helper);
        return 1;
    }
    ret = write_file("file1.txt", 16, 5, buf, helper);
    close_fs(helper);
    if (ret != 0){
        return 1;
    }
    if (compare_two_file_binary("before/12_file_data", "after/12_file_data", "file_data") != 0) {
        return 1;
    }
    if (compare_two_file_binary("before/12_directory_table", "after/12_directory_table", "directory_table") != 0) {
        return 1;
    }
    return 0;
}

int milestone_file_size() {
    void * helper = init_fs("before/14_file_data", "before/14_directory_table", "before/14_hash_data", 4);
    ssize_t ret = file_size("testfile", helper);
    int expected = 689;
    if (ret != expected) {
        close_fs(helper);
        return 1;
    }
    ret = file_size("nonexistent", helper);
    close_fs(helper);
    if (ret != -1) {
        return 1;
    }
    return 0;
}

int milestone_repack() {
    void * helper = init_fs("before/16_file_data", "before/16_directory_table", "before/16_hash_data", 4);
    repack(helper);
    if (compare_two_file_binary("before/16_file_data", "after/16_file_data", "file_data") != 0) {
        return 1;
    }
    if (compare_two_file_binary("before/16_directory_table", "after/16_directory_table", "directory_table") != 0) {
        return 1;
    }
    close_fs(helper);
    return 0;
}


int test_fletcher() {
    uint8_t data[16] = {13,34,54,65,3,2,0,23,45,78,84,24,67,89,34,22};
    uint8_t hash[16];
    uint8_t expected[16] = {128, 203, 172, 134, 219, 131, 163, 144, 97, 164, 60, 117, 31, 79, 174, 117};
    fletcher(data, 16, hash);
    if (memcmp(hash, expected, 16) != 0) {
        return 1;
    }
    return 0;
}

int hash_combo_test() {
    void * helper = init_fs("before/11_file_data", "before/11_directory_table", "before/11_hash_data", 4);
    char buf[] = "hash_com_test";
    int ret = write_file("test.o", 0, strlen(buf), buf, helper);
    assert(ret == 0);
    close_fs(helper);
    if (compare_two_file_binary("before/11_hash_data", "after/11_hash_data_step1", "hash_data") != 0)  {
        return 1;
    }
    helper = init_fs("before/11_file_data", "before/11_directory_table", "before/11_hash_data", 4);
    repack(helper);
    close_fs(helper);
    if (compare_two_file_binary("before/11_hash_data", "after/11_hash_data_step2", "hash_data") != 0)  {
        return 1;
    }
    return 0;
}

int test_compute_hash_tree() {
    void * helper = init_fs("before/13_file_data", "before/13_directory_table", "before/13_hash_data", 4);
    compute_hash_tree(helper);
    close_fs(helper);
    if (compare_two_file_binary("before/13_hash_data", "after/13_hash_data", "hash_data") != 0) {
        return 1;
    }
    return 0;
}

/****************************/

/* Helper function */
void test(int (*test_function) (), char * function_name) {
    int ret = test_function();
    if (ret == 0) {
        printf("Passed %s\n", function_name);
    } else {
        printf("Failed %s returned %d\n", function_name, ret);
    }
}

/************************/

int main(int argc, char * argv[]) {

//     You can use the TEST macro as TEST(x) to run a test function named "x"
    TEST(success);
    TEST(failure);
    TEST(no_operation);
    TEST(milestone_no_operation);
    TEST(milestone_create_file);
    TEST(milestone_create_file_exists);
    TEST(milestone_delete_file);
    TEST(milestone_delete_file_not_found);
    TEST(milestone_file_size);
    TEST(milestone_read_file);
    TEST(milestone_rename_file);
    TEST(milestone_rename_file_exists);
    TEST(milestone_write_file);
    TEST(milestone_repack);
    TEST(milestone_resize_file);
    TEST(milestone_resize_file_no_space);

    TEST(test_fletcher);
    TEST(hash_combo_test);
    TEST(test_compute_hash_tree);
    // Add more tests here
    return 0;
}
