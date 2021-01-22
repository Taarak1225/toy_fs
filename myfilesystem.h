#ifndef MYFILESYSTEM_H
#define MYFILESYSTEM_H
#include <sys/types.h>
#include <stdint.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>

#define BLOCK_SIZE 256
#define DIR_ITEM_SIZE 72
#define HASH_SIZE 16

typedef struct node node;
typedef struct list list;
typedef struct meta meta;
typedef struct super_block super_block;

pthread_mutex_t mylock;

struct node {
    meta *file;             // pointer to dirt_data
    node *next;
};

typedef struct list {
    node *head;
    uint32_t used_size;
    uint32_t max_size;
} list;

struct meta {
    char name[64];
    uint32_t offset;
    uint32_t length;
};

struct super_block {
    void *file_data;
    meta *dirt_data;
    void *hash_data;

    int n_processors;
    int file_size;          // the size of data file (in bytes)
    int dirt_size;          // the size of dirt file (in bytes)
    int hash_size;          // the size of hash file (in bytes)
    
    int block_num;          // the number of block, calculate by: file_size / BLOCK_SIZE
    int dirt_num;           // the number of directory item, calculate by: dirt_size / DIR_ITEM_SIZE
    int hash_num;           // the numver of hash, calculate by: hash_size / HASH_SIZE

    list *used_list;
};

/* initialize a list
 * ARGS(
 *     list ** used_list   :  &(super_block->used_list)
 * )
 * RETURN: -1 as failed, 0 as successed
 */
int list_init(list **used_list, uint32_t max_size);

/* add a node to list
 * ARGS(
 *     list * used_list    :  super_block->used_list
 *     meta * file         :  the meta struct pointer in directory (offset field is empty)
 *     uint32_t length     :  the length of file
 *     uint32_t * offset   :  return old offset of this file (write new offset to meta file)
 * )
 * RETURN: -1 as failed, 0 as successed, return offset by third parameter
 */
int add_node(list *used_list, meta *file, uint32_t length, uint32_t *offset);

/* remove a node from list
 * ARGS(
 *     list * used_list    :  super_block->used_list
 *     meta * file         :  the meta struct pointer in directory
 * )
 * RETURN: -1 as failed, 0 as successed
 */
int remove_node(list *used_list, meta *file);

/* destroy a list
 * ARGS(
 *     list * used_list    :  super_block->used_list
 * )
 * RETURN: -1 as failed, 0 as successed
 */
int list_destroy(list *used_list);

/* print node one by one, for debug
 * ARGS(
 *     list * used_list    :  super_block->used_list
 * )
 * NO RETURN, just for debug
 */
void print_node(list *used_list);

/* print directory one by one, for debug
 * ARGS(
 *     super_block * sys   :  super_block
 * )
 * NO RETURN, just for debug
 */
void print_dirt(super_block *sys);

/* ------------------------------------------------------------------------------------------------- */

/* initialize the super block of this file system
 * ARGS(
 *     char * f1           :  data file
 *     char * f2           :  directory file
 *     char * f3           :  hash file
 *     int n_processors    :  number of processors
 * )
 * RETURN: NULL as failed, helper pointer (super_block) as successed
 */
void * init_fs(char * f1, char * f2, char * f3, int n_processors);

/* destroy the file system, unmap the memory
 * ARGS(
 *     void * helper       :  super_block pointer
 * )
 * NO RETURN, terrible design
 */
void close_fs(void * helper);

/* create file
 * ARGS(
 *     char * filename     :  name of file
 *     size_t length       :  length of file
 *     void * helper       :  super_block
 * )
 * RETURN: 2 as no space for new file, 1 as filename exists, 0 as successed
 */
int create_file(char * filename, size_t length, void * helper);

/* resize file
 * ARGS(
 *     char * filename     :  name of file
 *     size_t length       :  new length of file
 *     void * helper       :  super_block
 * )
 * RETURN: 2 as no space for resize, 1 as file does not exist, 0 as successed
 */
int resize_file(char * filename, size_t length, void * helper);

/* no mutex version of resize_file */
int resize_file_nolock(char * filename, size_t length, void * helper);

/* repack file_data for super_block
 * ARGS(
 *     void * helper       :  super_block
 * )
 * NO RETURN
 */
void repack(void * helper);

/* no mutex version of repack */
void repack_nolock(void * helper);

/* delete file
 * ARGS(
 *     char * filename     :  name of file
 *     void * helper       :  super_block
 * )
 * RETURN: 1 as error, 0 as successed
 */
int delete_file(char * filename, void * helper);

/* rename file with oldname to newname
 * ARGS(
 *     char * oldname      :  oldname of file
 *     char * newname      :  newname of file
 *     void * helper       :  super_block
 * )
 * RETURN: 1 as error, 0 as successed
 */
int rename_file(char * oldname, char * newname, void * helper);

/* read count bytes from the file with given filename at offset from the start of the file
 * ARGS(
 *     char * filename     :  name of file
 *     size_t offset       :  offset of read location
 *     size_t count        :  size of data needed to be read
 *     void * buf          :  buffer to restore needed data
 *     void * helper       :  super_block
 * )
 * RETURN: 1 as file does not exist, 2 as offset or count invalid, 3 as hash check failed, 0 as successed
 */
int read_file(char * filename, size_t offset, size_t count, void * buf, void * helper);

/* write count bytes from the file with given filename at offset from the start of the file
 * ARGS(
 *     char * filename     :  name of file
 *     size_t offset       :  offset of read location
 *     size_t count        :  size of data needed to be write
 *     void * buf          :  buffer to restore data needed to be wrote
 *     void * helper       :  super_block
 * )   
 * RETURN: 1 as file does not exist, 2 as offset invalid, 3 as disk can not restore new file, 0 as successed
 */
int write_file(char * filename, size_t offset, size_t count, void * buf, void * helper);

/* get file_size
 * ARGS(
 *     char * filename     :  name of file
 *     void * helper       :  super_block
 * )
 * RETURN: size of file, -1 as error
 */
ssize_t file_size(char * filename, void * helper);

/* a variant of Fletcher hash algorithm
 * ARGS(
 *     uint8_t * buf       :  the data need to compute hash
 *     size_t length       :  length of data
 *     uint8_t * output    :  return the result, need to be malloc or calloc
 * )
 * NO RETURN, the result stored in output parameter
 */
void fletcher(uint8_t * buf, size_t length, uint8_t * output);

/* compute the entire Merkle hash tree
 * ARGS(
 *     void * helper       :  super_block
 * )
 * NO RETURN, directly write to super_block->hash_data
 */
void compute_hash_tree(void * helper);

/* no mutex version of compute_hash_tree */
void compute_hash_tree_nolock(void * helper);

/* compute the hash for the block at given block_offset, and update all affected hashed in Merkle tree
 * ARGS(
 *     size_t block_offset :  the offset of given block (counts blocks and not bytes)
 *     void * helper       :  super_block
 * )
 * NO RETURN, directly write to super_block->hash_data
 */
void compute_hash_block(size_t block_offset, void * helper);

/* no mutex version of compute_hash_block_nolock */
void compute_hash_block_nolock(size_t block_offset, void * helper);

/* this function will be used in read_file() to check the hash_data
 * ARGS(
 *     size_t block_offset :  the offset of given block (counts blocks and not bytes)
 *     void * helper       :  super_block
 * )
 * RETURN: -1 as failed, 0 as compliant, 1 as incompliant
 */
int hash_check(size_t block_offset, void * helper);

#endif