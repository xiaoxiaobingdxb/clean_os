#include "include/syscall.h"

#include "include/syscall_no.h"
#include "base.h"
void yield() { syscall0(SYSCALL_sched_yield); }

pid_t get_pid() { return syscall0(SYSCALL_get_pid); }

pid_t get_ppid() { return syscall0(SYSCALL_get_ppid); }

pid_t fork() { return syscall0(SYSCALL_fork); }

pid_t wait(int *status) { return syscall1(SYSCALL_wait, status); }

void exit(int status) { syscall0(SYSCALL_exit); }

int execve(const char *filename, char *const argv[], char *const envp[]) {
    return syscall3(SYSCALL_execv, filename, argv, envp);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, int offset) {
    return (void *)syscall6(SYSCALL_mmap, addr, length, prot, flags, fd,
                            offset);
}

void munmap(void *addr, size_t length) {
    syscall2(SYSCALL_munmap, addr, length);
}

uint32_t clone(int (*func)(void *), void *child_stack, int flags,
               void *func_arg) {
    syscall4(SYSCALL_clone, func, child_stack, flags, func_arg);
}

void deprecated_clone(const char *name, uint32_t priority, void *func,
                      void *func_arg) {}

int sysinfo(uint32_t pid, sys_info *info, int flags) {
    return syscall3(SYSCALL_sysinfo, pid, info, flags);
}

int ps(ps_info *ps, size_t count) { return syscall2(SYSCALL_ps, ps, count); }

void sleep(uint32_t ms_n) {
    syscall1(SYSCALL_sleep, ms_n);
}

fd_t dup(fd_t fd) { return syscall1(SYSCALL_dup, fd); }
fd_t dup2(fd_t dst, fd_t source) { return syscall2(SYSCALL_dup2, dst, source); }

fd_t open(const char *name, int flag) {
    return syscall2(SYSCALL_open, name, flag);
}

ssize_t write(fd_t fd, const void *buf, size_t size) {
    return syscall3(SYSCALL_write, fd, buf, size);
}
ssize_t read(fd_t fd, const void *buf, size_t size) {
    return syscall3(SYSCALL_read, fd, buf, size);
}

off_t lseek(fd_t fd, off_t offset, int whence) {
    switch (whence) {
    case SEEK_SET:
    case SEEK_CUR:
    case SEEK_END:
        return syscall3(SYSCALL_lseek, fd, offset, whence);
    default:
        break;
    }
    return EINVAL;
}

void close(fd_t fd) { syscall1(SYSCALL_close, fd); }

error stat(const char *name, void *data) {
    return syscall2(SYSCALL_stat, name, data);
}
error fstat(fd_t fd, void *data) {
    return syscall2(SYSCALL_fstat, fd, data);
}

error readdir(fd_t fd, dirent_t *dirent) {
    return syscall2(SYSCALL_readdir, fd, dirent);
}

error mkdir(const char *path) {
    return syscall1(SYSCALL_mkdir, path);
}
error rmdir(const char *path) {
    return syscall1(SYSCALL_rmdir, path);
}
error link(const char *new_path, const char *old_path) {
    return syscall2(SYSCALL_link, new_path, old_path);
}
error symlink(const char *new_path, const char *old_path) {
    return syscall2(SYSCALL_symlink, new_path, old_path);
}
error unlink(const char *path) {
    return syscall1(SYSCALL_unlink, path);
}

void timestamp(const timespec_t *timestamp) {
    syscall1(SYSCALL_timestamp, timestamp);
}