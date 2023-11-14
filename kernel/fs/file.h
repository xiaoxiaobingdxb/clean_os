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
    uint32_t pos;
    int ref;
} file_t;

void init_file_table();
file_t *alloc_file();
void ref_file(file_t *file);
void free_file(file_t *file);

#endif // FS_FILE_H
