#if !defined(FS_FS_H)
#define FS_FS_H

#include "common/types/basic.h"

void init_fs();

ssize_t sys_write(int fd, const void *buf, size_t size);

ssize_t sys_read(int fd, const void *buf, size_t size);


#endif // FS_FS_H
