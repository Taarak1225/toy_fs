#include "myfilesystem.h"

// TODO

int list_init(list **used_list, uint32_t max_size) {
    (*used_list) = (list *)malloc(sizeof(list));
    if ((*used_list) == NULL) {
        printf("malloc list error\n");
        return -1;
    }

    (*used_list)->head = NULL;
    (*used_list)->used_size = 0;
    (*used_list)->max_size = max_size;

    return 0;
}

int add_node(list *used_list, meta *file, uint32_t length, uint32_t *offset) {
    node *new_node = (node *)malloc(sizeof(node));
    new_node->file = file;
    node *pointer = used_list->head;

    if (pointer == NULL) {
        used_list->head = new_node;
        new_node->next = NULL;
        used_list->used_size += length;

        *offset = file->offset;
        file->offset = 0;
        file->length = length;

        return 0;
    }

    uint32_t begin = 0, end = 0;
    while (pointer != NULL) {
        begin = pointer->file->offset + pointer->file->length;
        end = (pointer->next == NULL) ? used_list->max_size : pointer->next->file->offset;

        if ((end - begin) >= new_node->file->length) {
            new_node->next = pointer->next;
            pointer->next = new_node;
            used_list->used_size += length;

            *offset = file->offset;
            file->offset = begin;
            file->length = length;

            return 0;
        }
        pointer = pointer->next;
    }
    return -1;
}

int remove_node(list *used_list, meta *file) {
    node *pointer = used_list->head;

    if (pointer->file == file) {
        used_list->head = pointer->next;
        free(pointer);
        used_list->used_size -= file->length;
        return 0;
    }

    while ((pointer->next->file != file) && (pointer->next != NULL)) {
        pointer = pointer->next;
    }
    if (pointer->next == NULL) {
        printf("target file can not be found\n");
        return -1;
    }
    node *tofree = pointer->next;
    pointer->next = tofree->next;
    free(tofree);
    used_list->used_size -= file->length;

    return 0;
}

int list_destroy(list *used_list) {
    if (used_list == NULL) {
        printf("parameter is NULL\n");
        return -1;
    }

    node *curr, *next;
    for (curr = used_list->head; curr != NULL; curr = next) {
        next = curr->next;
        free(curr);
    }

    free(used_list);
    return 0;
}

void print_node(list *used_list) {
    int i, count;
    node *curr;
    printf("List: \n");

    for (i = 0, count = 0, curr = used_list->head; curr != NULL; curr = curr->next, i++) {
        printf("    #%02dnode, meta{%s, %d, %d}, ", i + 1, curr->file->name, curr->file->offset, curr->file->length);
        printf("[%d, %d] ", curr->file->offset, curr->file->offset + curr->file->length);
        printf("-> \n");
        count++;
    }
    printf("    NULL\n");
    printf("%d nodes in list, used_size = %d / %d\n\n", count, used_list->used_size, used_list->max_size);

    return;
}

void print_dirt(super_block *sys) {
    meta *dirt = sys->dirt_data;
    printf("Directory: \n");

    int i = 0, count = 0;
    for (i = 0; i < sys->dirt_num; i++) {
        if (dirt[i].name[0] == '\0') {
            continue;
        }
        printf("    [%03d]%s, offset = %d, length = %d\n", i + 1, dirt[i].name, dirt[i].offset, dirt[i].length);
        count++;
    }
    if (count == 0) {
        printf("    NULL\n");
    }
    printf("%d files in directory\n", count);

    return;
}

void * init_fs(char * f1, char * f2, char * f3, int n_processors) {
    if (f1 == NULL || f2 == NULL || f3 == NULL) {
        printf("parameters error\n");
        return NULL;
    }
    
    int file_fd, dirt_fd, hash_fd;
    struct stat st;
    super_block *helper = (super_block *)malloc(sizeof(super_block));
    helper->n_processors = n_processors;

    /* initialize file_data and related */
    if (file_fd = open(f1, O_RDWR), file_fd < 0) {
        printf("open %s failed\n", f1);
        printf("Message: %s\n", strerror(errno));
        return NULL;
    }
    if (stat(f1, &st) < 0) {
        printf("get %s status failed\n", f2);
        return NULL;
    }
    helper->file_data = mmap(0, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd, 0);
    if (helper->file_data == MAP_FAILED) {
        printf("mmap to file_data failed\n");
        return NULL;
    }
    helper->file_size = st.st_size;
    helper->block_num = st.st_size / BLOCK_SIZE;

    /* initialize used_list and related */
    if (list_init(&(helper->used_list), helper->file_size) < 0) {
        printf("list_init failed\n");
        return NULL;
    }

    /* initialize dirt_data and related */
    if (dirt_fd = open(f2, O_RDWR), dirt_fd < 0) {
        printf("open %s failed\n", f2);
        printf("Message: %s\n", strerror(errno));
        return NULL;
    }
    if (stat(f2, &st) < 0) {
        printf("get %s status failed\n", f2);
        return NULL;
    }
    helper->dirt_data = (meta *)mmap(0, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, dirt_fd, 0);
    if ((void *)helper->dirt_data == MAP_FAILED) {
        printf("mmap to dirt_data failed\n");
        return NULL;
    }
    helper->dirt_size = st.st_size;
    helper->dirt_num = st.st_size / DIR_ITEM_SIZE;

    /* initialize hash_data and related */
    if (hash_fd = open(f3, O_RDWR), hash_fd < 0) {
        printf("open %s failed\n", f3);
        printf("Message: %s\n", strerror(errno));
        return NULL;
    }
    if (stat(f3, &st) < 0) {
        printf("get %s status failed\n", f3);
        return NULL;
    }
    helper->hash_data = mmap(0, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, hash_fd, 0);
    if (helper->hash_data == MAP_FAILED) {
        printf("mmap to hash_data failed\n");
        return NULL;
    }
    helper->hash_size = st.st_size;
    helper->hash_num = st.st_size / HASH_SIZE;

    if (pthread_mutex_init(&mylock, NULL) != 0) {
        printf("pthread_mutex_init failed\n");
        return NULL;
    }

    close(file_fd);
    close(dirt_fd);
    close(hash_fd);

    return helper;
}

void close_fs(void * helper) {
    super_block *sys = (super_block *)helper;

    list_destroy(sys->used_list);

    if (munmap(sys->file_data, sys->file_size) < 0) {
        printf("munmap to file_data failed\n");
        // this function have no return value, can not communicate to calling function
    }
    if (munmap(sys->dirt_data, sys->dirt_size) < 0) {
        printf("munmap to dirt_data failed\n");
        // as mentioned above
    }
    if (munmap(sys->hash_data, sys->hash_size) < 0) {
        printf("munmap to hash_data failed\n");
        // as mentioned above
    }

    pthread_mutex_destroy(&mylock);
    free(sys);
    // as mentioned above
}

void fletcher(uint8_t * buf, size_t length, uint8_t * output) {
    // if data in buf is not a multiple of 4 bytes, pad it with zero bytes
    uint32_t size = (length % 4 == 0) ? (length / 4) : (length / 4 + 1);
    uint32_t *temp = (uint32_t *)calloc(size, sizeof(uint32_t));
    memcpy(temp, buf, length);

    uint64_t a = 0;
    uint64_t b = 0;
    uint64_t c = 0;
    uint64_t d = 0;

    for (uint32_t i = 0; i < size; i++) {
        a = (a + temp[i]) % UINT32_MAX;
        b = (b + a) % UINT32_MAX;
        c = (c + b) % UINT32_MAX;
        d = (d + c) % UINT32_MAX;
    }

    memcpy(output, &a, 4);
    memcpy(output + 4, &b, 4);
    memcpy(output + 8, &c, 4);
    memcpy(output + 12, &d, 4);

    free(temp);
    return;
}

void compute_hash_tree(void * helper) {
    pthread_mutex_lock(&mylock);
    super_block *sys = (super_block *)helper;

    if (sys == NULL) {
        printf("invalid parameter\n");
        pthread_mutex_unlock(&mylock);
        return;
    }

    uint8_t *buf = (uint8_t *)malloc(HASH_SIZE * 2);

    int start = sys->block_num - 1;     // the corresponding position of the low-left corner
    int end = start * 2;                // the corresponding position of the low-right corner

    // the bottom layer, directly data, calculated separately
    for (int i = 0; i < sys->block_num; i++) {
        fletcher(sys->file_data + i * BLOCK_SIZE, BLOCK_SIZE, sys->hash_data + (start + i) * HASH_SIZE);
    }

    // middle layer and top layer, automatically
    end = start - 1;
    start = start / 2;
    while (end >= 0) {
        for (int i = start; i <= end; i++) {
            int lchild = i * 2 + 1;
            int rchild = i * 2 + 2;
            memcpy(buf, sys->hash_data + lchild * HASH_SIZE, HASH_SIZE);
            memcpy(buf + HASH_SIZE, sys->hash_data + rchild * HASH_SIZE, HASH_SIZE);
            fletcher(buf, HASH_SIZE * 2, sys->hash_data + i * HASH_SIZE);
        }
        end = start - 1;
        start = start / 2;
    }

    free(buf);
    pthread_mutex_unlock(&mylock);
    return;
}

void compute_hash_tree_nolock(void * helper) {
    super_block *sys = (super_block *)helper;

    if (sys == NULL) {
        printf("invalid parameter\n");
        return;
    }

    uint8_t *buf = (uint8_t *)malloc(HASH_SIZE * 2);

    int start = sys->block_num - 1;
    int end = start * 2;

    for (int i = 0; i < sys->block_num; i++) {
        fletcher(sys->file_data + i * BLOCK_SIZE, BLOCK_SIZE, sys->hash_data + (start + i) * HASH_SIZE);
    }

    end = start - 1;
    start = start / 2;
    while (end >= 0) {
        for (int i = start; i <= end; i++) {
            int lchild = i * 2 + 1;
            int rchild = i * 2 + 2;
            memcpy(buf, sys->hash_data + lchild * HASH_SIZE, HASH_SIZE);
            memcpy(buf + HASH_SIZE, sys->hash_data + rchild * HASH_SIZE, HASH_SIZE);
            fletcher(buf, HASH_SIZE * 2, sys->hash_data + i * HASH_SIZE);
        }
        end = start - 1;
        start = start / 2;
    }

    free(buf);
    return;
}

void compute_hash_block(size_t block_offset, void * helper) {
    pthread_mutex_lock(&mylock);
    super_block *sys = (super_block *)helper;

    if (sys == NULL) {
        printf("invalid parameter\n");
        pthread_mutex_unlock(&mylock);
        return;
    }

    uint8_t buf[HASH_SIZE * 2];
    int index = sys->block_num - 1 + block_offset;
    // compute the hash of given block, then climb up to the root
    fletcher(sys->file_data + block_offset * BLOCK_SIZE, BLOCK_SIZE, sys->hash_data + index * HASH_SIZE);

    index = (index - 1) / 2;    // climb up to parent
    int flag = 0;               // flag = 1 if reached the root, else 0
    while (flag != 1) {
        if (index == 0) {
            flag = 1;
        }
        memcpy(buf, sys->hash_data + (index * 2 + 1) * HASH_SIZE, HASH_SIZE);
        memcpy(buf + HASH_SIZE, sys->hash_data + (index * 2 + 2) * HASH_SIZE, HASH_SIZE);
        fletcher(buf, HASH_SIZE * 2, sys->hash_data + index * HASH_SIZE);

        index = (index - 1) / 2;
    }

    pthread_mutex_unlock(&mylock);
    return;
}

void compute_hash_block_nolock(size_t block_offset, void * helper) {
    super_block *sys = (super_block *)helper;

    if (sys == NULL) {
        printf("invalid parameter\n");
        return;
    }

    uint8_t buf[HASH_SIZE * 2];
    int index = sys->block_num - 1 + block_offset;
    fletcher(sys->file_data + block_offset * BLOCK_SIZE, BLOCK_SIZE, sys->hash_data + index * HASH_SIZE);

    index = (index - 1) / 2;
    int flag = 0;
    while (flag != 1) {
        if (index == 0) {
            flag = 1;
        }
        memcpy(buf, sys->hash_data + (index * 2 + 1) * HASH_SIZE, HASH_SIZE);
        memcpy(buf + HASH_SIZE, sys->hash_data + (index * 2 + 2) * HASH_SIZE, HASH_SIZE);
        fletcher(buf, HASH_SIZE * 2, sys->hash_data + index * HASH_SIZE);

        index = (index - 1) / 2;
    }

    return;
}

int hash_check(size_t block_offset, void * helper) {
    super_block *sys = (super_block *)helper;

    if (sys == NULL || block_offset < 0) {
        printf("invalid parameter\n");
        return -1;
    }

    uint8_t buf32[HASH_SIZE * 2];
    uint8_t buf16[HASH_SIZE];

    // compute the hash of given block, compare with corresponding item in hash_data
    int index = sys->block_num - 1 + block_offset;
    fletcher(sys->file_data + block_offset * BLOCK_SIZE, BLOCK_SIZE, buf16);
    if (memcmp(buf16, sys->hash_data + index * HASH_SIZE, HASH_SIZE) != 0) {
        printf("data incompliant\n");
        return 1;
    }

    // climb up to the root
    index = (index - 1) / 2;
    int flag = 0;               // flag = 1 if reached the root, else 0
    while (flag != 1) {
        if (index == 0) {
            flag = 1;
        }
        memcpy(buf32, sys->hash_data + (index * 2 + 1) * HASH_SIZE, HASH_SIZE * 2);
        fletcher(buf32, HASH_SIZE * 2, buf16);
        if (memcmp(buf16, sys->hash_data + index * HASH_SIZE, HASH_SIZE) != 0) {
            printf("data incompliant\n");
            return 1;
        }

        index = (index - 1) / 2;
    }

    return 0;
}

int create_file(char * filename, size_t length, void * helper) {
    pthread_mutex_lock(&mylock);
    super_block *sys = (super_block *)helper;

    // check file_data
    if (length > (int64_t)(sys->file_size - sys->used_list->used_size)) {
        pthread_mutex_unlock(&mylock);
        return 2;           // no space in file_data
    }

    // check dirt_data
    meta *spare_loc = NULL;
    for (int i = 0; i < sys->dirt_num; i++) {
        if (sys->dirt_data[i].name[0] == '\0') {
            if (spare_loc == NULL) {
                spare_loc = sys->dirt_data + i;
            }
        }
        else {
            if (strcmp(filename, sys->dirt_data[i].name) == 0) {
                pthread_mutex_unlock(&mylock);
                return 1;   // same file in dirt_data
            }
        }
    }
    if (spare_loc == NULL) {
        pthread_mutex_unlock(&mylock);
        return 2;           // no space in dirt_data
    }

    // insert directory item in dirt_data
    if (strlen(filename) > 63) {
        filename[63] = '\0';
    }
    strncpy((char *)spare_loc, filename, 63);
    spare_loc->length = length;

    // insert node in used_list
    uint32_t old_offset;
    if (add_node(sys->used_list, spare_loc, length, &old_offset) < 0) {
        repack_nolock(sys);
        add_node(sys->used_list, spare_loc, length, &old_offset);
    }

    pthread_mutex_unlock(&mylock);
    return 0;
}

int resize_file(char * filename, size_t length, void * helper) {
    pthread_mutex_lock(&mylock);
    super_block *sys = (super_block *)helper;

    // figure out if the file exists
    int index = -1;
    for (int i = 0; i < sys->dirt_num; i++) {
        if (strcmp(filename, sys->dirt_data[i].name) == 0) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        pthread_mutex_unlock(&mylock);
        return 1;
    }

    // figure out if the length is valid , eg: (offset + length) < offset
    if ((int64_t)(sys->dirt_data[index].offset + length) < (int64_t)sys->dirt_data[index].offset) {
        pthread_mutex_unlock(&mylock);
        return 2;
    }

    // figure out if the space is enough
    int64_t diff = length - sys->dirt_data[index].length;
    if (diff > (int64_t)(sys->file_size - sys->used_list->used_size)) {
        pthread_mutex_unlock(&mylock);
        return 2;
    }

    // resize the file, depends on increase or decrease (or no change)
    if (diff == 0) {
        pthread_mutex_unlock(&mylock);
        return 0;
    }
    else if (diff < 0) {
        sys->dirt_data[index].length = length;
        sys->used_list->used_size += diff;

        pthread_mutex_unlock(&mylock);
        return 0;
    }
    else {
        node *cur = sys->used_list->head;
        while (cur->file != &(sys->dirt_data[index])) {
            cur = cur->next;
        }

        int64_t space = (cur->next == NULL) ? (sys->file_size - cur->file->offset) : (cur->next->file->offset - cur->file->offset);
        if (space > (int64_t)length) {
            memset(sys->file_data + cur->file->offset + cur->file->length, 0, length - cur->file->length);
            sys->dirt_data[index].length = length;
            sys->used_list->used_size += diff;

            pthread_mutex_unlock(&mylock);
            return 0;
        }
        else {
            // backup file
            uint32_t old_length = cur->file->length;
            void *temp_buf = malloc(old_length);
            memcpy(temp_buf, sys->file_data + cur->file->offset, old_length);

            // remove node and repack
            remove_node(sys->used_list, &(sys->dirt_data[index]));
            repack_nolock(sys);

            // allocate new location and length
            uint32_t old_offset;
            sys->dirt_data[index].length = length;
            add_node(sys->used_list, &(sys->dirt_data[index]), sys->dirt_data[index].length, &old_offset);

            // restore backup data and set new space to 0
            memcpy(sys->file_data + sys->dirt_data[index].offset, temp_buf, old_length);
            memset(sys->file_data + sys->dirt_data[index].offset + old_length, 0, length - old_length);
            free(temp_buf);
            sys->used_list->used_size += diff;

            pthread_mutex_unlock(&mylock);
            return 0;
        }
    }
}

int resize_file_nolock(char * filename, size_t length, void * helper) {
    super_block *sys = (super_block *)helper;

    // figure out if the file exists
    int index = -1;
    for (int i = 0; i < sys->dirt_num; i++) {
        if (strcmp(filename, sys->dirt_data[i].name) == 0) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        return 1;
    }

    // figure out if the space is enough
    int64_t diff = length - sys->dirt_data[index].length;
    if (diff > (int64_t)(sys->file_size - sys->used_list->used_size)) {
        return 2;
    }

    // resize the file, depends on increase or decrease (or no change)
    if (diff == 0) {
        return 0;
    }
    else if (diff < 0) {
        sys->dirt_data[index].length = length;
        sys->used_list->used_size += diff;

        return 0;
    }
    else {
        node *cur = sys->used_list->head;
        while (cur->file != &(sys->dirt_data[index])) {
            cur = cur->next;
        }

        int64_t space = (cur->next == NULL) ? (sys->file_size - cur->file->offset) : (cur->next->file->offset - cur->file->offset);
        if (space > (int64_t)length) {
            memset(sys->file_data + cur->file->offset + cur->file->length, 0, length - cur->file->length);
            sys->dirt_data[index].length = length;
            sys->used_list->used_size += diff;

            return 0;
        }
        else {
            // backup file
            uint32_t old_length = cur->file->length;
            void *temp_buf = malloc(old_length);
            memcpy(temp_buf, sys->file_data + cur->file->offset, old_length);

            // remove node and repack
            remove_node(sys->used_list, &(sys->dirt_data[index]));
            repack_nolock(sys);

            // allocate new location and length
            uint32_t old_offset;
            sys->dirt_data[index].length = length;
            add_node(sys->used_list, &(sys->dirt_data[index]), sys->dirt_data[index].length, &old_offset);

            // restore backup data and set new space to 0
            memcpy(sys->file_data + sys->dirt_data[index].offset, temp_buf, old_length);
            memset(sys->file_data + sys->dirt_data[index].offset + old_length, 0, length - old_length);
            free(temp_buf);
            sys->used_list->used_size += diff;

            return 0;
        }
    }
}

void repack(void * helper) {
    pthread_mutex_lock(&mylock);
    super_block *sys = (super_block *)helper;

    if ((sys == NULL) || (sys->used_list->head == NULL)) {
        pthread_mutex_unlock(&mylock);
        return;
    }

    uint32_t new_loc;
    node *cur = sys->used_list->head;
    while (cur->next != NULL) {
        node *to_move = cur->next;
        new_loc = cur->file->offset + cur->file->length;
        memcpy(sys->file_data + new_loc, sys->file_data + to_move->file->offset, to_move->file->length);
        to_move->file->offset = new_loc;

        cur = cur->next;
    }

    compute_hash_tree_nolock(sys);

    pthread_mutex_unlock(&mylock);
    return;
}

void repack_nolock(void * helper) {
    super_block *sys = (super_block *)helper;

    if ((sys == NULL) || (sys->used_list->head == NULL)) {
        return;
    }

    uint32_t new_loc;
    node *cur = sys->used_list->head;
    while (cur->next != NULL) {
        node *to_move = cur->next;
        new_loc = cur->file->offset + cur->file->length;
        memcpy(sys->file_data + new_loc, sys->file_data + to_move->file->offset, to_move->file->length);
        to_move->file->offset = new_loc;

        cur = cur->next;
    }

    compute_hash_tree_nolock(sys);

    return;
}

int delete_file(char * filename, void * helper) {
    pthread_mutex_lock(&mylock);
    super_block *sys = (super_block *)helper;

    if (sys == NULL) {
        pthread_mutex_unlock(&mylock);
        return 1;
    }

    int index = -1;
    for (int i = 0; i < sys->dirt_num; i++) {
        if (strcmp(filename, sys->dirt_data[i].name) == 0) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        pthread_mutex_unlock(&mylock);
        return 1;
    }

    if (remove_node(sys->used_list, &(sys->dirt_data[index])) != 0) {
        pthread_mutex_unlock(&mylock);
        return 1;
    }
    sys->dirt_data[index].name[0] = '\0';

    pthread_mutex_unlock(&mylock);
    return 0;
}

int rename_file(char * oldname, char * newname, void * helper) {
    pthread_mutex_lock(&mylock);
    super_block *sys = (super_block *)helper;

    if ((oldname == NULL) || (newname == NULL) || (sys == NULL)) {
        pthread_mutex_unlock(&mylock);
        return 1;
    }
    if ((strlen(oldname) > 63) || (strlen(newname) > 63)) {
        pthread_mutex_unlock(&mylock);
        return 1;
    }

    int index = -1;
    for (int i = 0; i < sys->dirt_num; i++) {
        if (strcmp(oldname, sys->dirt_data[i].name) == 0) {
            index = i;
        }
        if (strcmp(newname, sys->dirt_data[i].name) == 0) {
            pthread_mutex_unlock(&mylock);
            return 1;
        }
    }
    if (index == -1) {
        pthread_mutex_unlock(&mylock);
        return 1;
    }

    strncpy(sys->dirt_data[index].name, newname, 63);

    pthread_mutex_unlock(&mylock);
    return 0;
}

int read_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {
    pthread_mutex_lock(&mylock);
    super_block *sys = (super_block *)helper;

    if ((sys == NULL) || (filename == NULL)) {
        pthread_mutex_unlock(&mylock);
        return -1;
    }

    int index = -1;
    for (int i = 0; i < sys->dirt_num; i++) {
        if (strcmp(filename, sys->dirt_data[i].name) == 0) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        pthread_mutex_unlock(&mylock);
        return 1;
    }
    meta *f = &(sys->dirt_data[index]);

    size_t begin = sys->dirt_data[index].offset + offset;
    size_t end = begin + count;
    if ((begin > end) || (begin < (int64_t)f->offset) || (begin > (int64_t)(f->offset + f->length))) {
        pthread_mutex_unlock(&mylock);
        return 2;
    }
    if (end > (int64_t)(f->offset + f->length)) {
        pthread_mutex_unlock(&mylock);
        return 2;
    }
    if (begin == end) {
        pthread_mutex_unlock(&mylock);
        return 0;
    }

    // verify contents of hash data and get data
    uint32_t begin_block = begin / BLOCK_SIZE;
    uint32_t end_block = end / BLOCK_SIZE;
    for (uint32_t i = begin_block; i <= end_block; i++) {
        if (hash_check(i, sys) != 0) {
            pthread_mutex_unlock(&mylock);
            return 3;
        }
    }
    memcpy(buf, sys->file_data + begin, count);

    pthread_mutex_unlock(&mylock);
    return 0;
}

int write_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {
    pthread_mutex_lock(&mylock);
    super_block *sys = (super_block *)helper;

    if ((filename == NULL) || (sys == NULL)) {
        pthread_mutex_unlock(&mylock);
        return -1;
    }

    int index = -1;
    for (int i = 0; i < sys->dirt_num; i++) {
        if (strcmp(filename, sys->dirt_data[i].name) == 0) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        pthread_mutex_unlock(&mylock);
        return 1;
    }
    meta *f = &(sys->dirt_data[index]);

    size_t begin = f->offset + offset;
    if ((begin < (int64_t)f->offset) || (begin > (int64_t)(f->offset + f->length))) {
        pthread_mutex_unlock(&mylock);
        return 2;
    }

    // if need to resize file
    if ((offset + count) > (int64_t)f->length) {
        if (resize_file_nolock(f->name, offset + count, sys) != 0) {
            pthread_mutex_unlock(&mylock);
            return 3;
        }
    }
    begin = f->offset + offset;

    memcpy(sys->file_data + begin, buf, count);
    compute_hash_tree_nolock(sys);

    pthread_mutex_unlock(&mylock);
    return 0;
}

ssize_t file_size(char * filename, void * helper) {
    pthread_mutex_lock(&mylock);
    super_block *sys = (super_block *)helper;

    if (sys == NULL) {
        pthread_mutex_unlock(&mylock);
        return -1;
    }

    ssize_t length = -1;
    for (int i = 0; i < sys->dirt_num; i++) {
        if (strcmp(filename, sys->dirt_data[i].name) == 0) {
            length = sys->dirt_data[i].length;
            break;
        }
    }
    if (length == -1) {
        pthread_mutex_unlock(&mylock);
        return -1;
    }

    pthread_mutex_unlock(&mylock);
    return length;
}