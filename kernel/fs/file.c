#include "file.h"
#include "common/lib/string.h"

#define FILE_TABLE_SIZE 2048
file_t file_table[FILE_TABLE_SIZE];

void init_file_table() {
    memset(file_table, 0, sizeof(file_table));
}

file_t *alloc_file() {
    for (int i = 0; i < FILE_TABLE_SIZE; i++) {
        file_t *file = file_table + i;
        if (file->ref == 0) {
            file->ref = 1;
            return file;
        }
    }
    return NULL;
}

void ref_file(file_t *file) {
    file->ref++;
}

void free_file(file_t *file) {
    file->ref--;
}