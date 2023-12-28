#ifndef SYSCALL_SYSCALL_USER_H
#define SYSCALL_SYSCALL_USER_H
#include "include/syscall.h"
// #include "../thread/model.h"
// #include "../thread/thread.h"
// #include "common/tool/datetime.h"
// #include "common/types/basic.h"

// #define PROT_NONE 0x0  /* page can not be accessed */
// #define PROT_READ 0x1  /* page can be read */
// #define PROT_WRITE 0x2 /* page can be written */
// #define PROT_EXEC 0x4  /* page can be executed */

// #define MAP_SHARED 0x01    /* Share changes */
// #define MAP_PRIVATE 0x02   /* Changes are private */
// #define MAP_ANONYMOUS 0x20 /* don't use a file */

// void *mmap(void *addr, size_t length, int prot, int flags, int fd, int offset);
// static inline void *mmap_anonymous(size_t length) {
//     return mmap(NULL, length, PROT_READ | PROT_READ, MAP_ANONYMOUS, 0, 0);
// }
// void munmap(void *addr, size_t size);

// void yield();
// pid_t get_pid();
// pid_t get_ppid();
// pid_t fork();
// pid_t wait(int *status);
// void exit(int status);
// int execve(const char *filename, char *const argv[], char *const envp[]);

// #define CLONE_VM (1 << 0)  // set if vm shared between process
// #define CLONE_PARENT \
//     (1 << 1)  // set if want the same parent, also create a new process as the
//               // cloner' brother
// #define CLONE_VFORK \
//     (1 << 2)  // set if block current process until child process exit
// pid_t clone(int (*func)(void *), void *child_stack, int flags, void *func_arg);

// void deprecated_clone(const char *name, uint32_t priority, void *func,
//                       void *func_arg);

// int sysinfo(uint32_t pid, sys_info *info);
// int ps(ps_info *ps, size_t count);

// #define FILENO_UNKNOWN -1
// #define STDIN_FILENO 0
// #define STDOUT_FILENO 1
// #define STDERR_FILENO 2
// #define O_CREAT (1 << 0)
// #define O_RDONLY (1 << 1)
// #define O_WRONLY (1 << 2)
// #define O_RDWR (1 << 3)
// #define O_APPEND (1 << 4)
// #define O_TRUNC (1 << 5)
// #define O_EXCL (1 << 6)

// fd_t open(const char *name, int flag);
// fd_t dup(fd_t fd);
// fd_t dup2(fd_t dst, fd_t source);
// ssize_t write(fd_t fd, const void *buf, size_t size);
// ssize_t read(fd_t fd, const void *buf, size_t size);

// #define SEEK_SET 0    // set position = offset
// #define SEEK_CUR 1    // position += offset
// #define SEEK_END 2    // position = end
// #define EBADF -2      // fd is not a file descriptor
// #define EINVAL -3     // whence is invalid
// #define EOVERFLOW -4  // return offset is overflow sizeof(off_t)
// #define ESPIPE -5
// off_t lseek(fd_t fd, off_t offset, int whence);
// void close(fd_t fd);

// typedef struct {
//     time64_t tv_sec;
//     time64_t tv_nsec;
// } timespec_t;
// typedef struct {
//     dev_id_t dev_id;
//     size_t size;
//     size_t block_size;
//     size_t block_count;
//     timespec_t create_time;
//     timespec_t update_time;
// } stat_t;
// error stat(const char *name, void *data);
// error fstat(fd_t fd, void *data);

// typedef struct {
//     off_t offset;
//     uint32_t size;
//     file_type type;
//     char name[64];
// } dirent_t;
// error readdir(fd_t fd, dirent_t *dirent);

// error mkdir(const char *path);
// error rmdir(const char *path);
// error link(const char *new_path, const char *old_path);
// error symlink(const char *new_path, const char *old_path);
// error unlink(const char *path);

#endif