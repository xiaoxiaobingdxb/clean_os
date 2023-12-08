#if !defined(FS_FILE_H)
#define FS_FILE_H

#include "common/types/basic.h"
#include "../device/device.h"

typedef int fd_t;
typedef int mode_t;

#define FILE_NAME_SIZE 64
struct _fs_desc_t;
typedef enum {
    UNKNOWN,
    FILE_TTY,
    FILE_FILE,
    FILE_DIR,
} file_type;
typedef struct {
    struct _fs_desc_t *desc;
    dev_id_t dev_id;
    char name[FILE_NAME_SIZE];
    file_type type;
    mode_t mode;
    int ref;

    uint32_t file_idx;
    uint32_t size;

    uint32_t pos;
    uint32_t block_start;
    uint32_t block_cur;
} file_t;

void init_file_table();
file_t *alloc_file();
void ref_file(file_t *file);
void free_file(file_t *file);

/**
 * @brief /a/b/c -> b/c
*/
static inline const char* path_next_child(const char *path) {
    if (*path == '/') {
        path++;
    }
    while (*path != '/' && *path != 0) {
        path++;
    }
    if (*path != 0) {
        path++;
    }
    return (const char*)path;
}

#endif // FS_FILE_H
