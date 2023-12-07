#include "../memory/malloc/malloc.h"
#include "../syscall/syscall_user.h"
#include "common/lib/stdio.h"
#include "common/lib/string.h"
#include "../io/std.h"

void init_tty() {
    fd_t fd = open("/dev/tty0", 0);
    dup2(STDIN_FILENO, fd);
    dup2(STDOUT_FILENO, fd);
    dup2(STDERR_FILENO, fd);
}
void test_tty() {
    init_tty();
    char *str = "test_stdout\n";
    ssize_t count = write(STDOUT_FILENO, str, strlen(str));
    str = "test_stderr\n";
    count = write(STDERR_FILENO, str, strlen(str));
    str = mmap_anonymous(10);
    memset(str, 0, 10);
    sprintf(str, "test_%d\n", 10);
    write(STDOUT_FILENO, str, strlen(str));
    str = "end";
}

void test_kbd() {
    init_tty();
    int buf_size = 32;
    char *buf = mmap_anonymous(buf_size);
    ssize_t count = 0;
    size_t get_size = 0;
    while ((count = read(STDIN_FILENO, (void *)buf, buf_size)) > 0) {
        get_size = count;
    }
}

void test_disk() {
    init_tty();
    // ========================================================>
    fd_t fd = open("/home/test.txt", O_RDONLY);
    size_t buf_size = 512;
    byte_t *read_buf = malloc(buf_size);
    memset(read_buf, 0, buf_size);
    ssize_t read_size = read(fd, read_buf, buf_size);
    char *out_buf = (char *)malloc(buf_size);
    memset(out_buf, 0, buf_size);
    sprintf(out_buf, "read disk size=%d, value=%s\n", read_size, read_buf);
    write(STDOUT_FILENO, out_buf, strlen(out_buf));
    stat_t var_stat;
    if (fstat(fd, &var_stat) == 0) {
        memset(out_buf, 0, buf_size);
        uint32_t create_time_h = (uint32_t)(var_stat.create_time.tv_sec >> 32);
        uint32_t create_time_l =
            (uint32_t)(var_stat.create_time.tv_sec & 0xffffffff);
        uint32_t update_time_h = (uint32_t)(var_stat.update_time.tv_sec >> 32);
        uint32_t update_time_l =
            (uint32_t)(var_stat.update_time.tv_sec & 0xffffffff);
        sprintf(
            out_buf,
            "file stat "
            "dev_id=%d,size=%d,block_size=%d,block_count=%d,create_time=%d%d,"
            "update_time=%d%d\n",
            var_stat.dev_id, var_stat.size, var_stat.block_size,
            var_stat.block_count, var_stat.create_time.tv_sec >> 32,
            var_stat.create_time.tv_sec & 0xffffffff,
            var_stat.update_time.tv_sec >> 32,
            var_stat.update_time.tv_sec & 0xffffffff);
        write(STDOUT_FILENO, out_buf, strlen(out_buf));
    }
    close(fd);
    // ========================================================>
    fd = open("/home/ttest.txt", O_RDWR | O_CREAT);

    // memset(read_buf, 0, sizeof(read_buf));
    // read_size = read(fd, read_buf, 512);
    // memset(out_buf, 0, sizeof(out_buf));
    // sprintf(out_buf, "before write, read disk size=%d, value=%s\n",
    // read_size, read_buf); write(STDOUT_FILENO, out_buf, strlen(out_buf));

    const char *write_data = "test\n";
    write(fd, write_data, strlen(write_data));

    lseek(fd, 0, SEEK_SET);
    memset(read_buf, 0, buf_size);
    read_size = read(fd, read_buf, buf_size);
    memset(out_buf, 0, buf_size);
    sprintf(out_buf, "after write, read disk size=%d, value=%s\n", read_size,
            read_buf);
    write(STDOUT_FILENO, out_buf, strlen(out_buf));

    if (fstat(fd, &var_stat) == 0) {
        memset(out_buf, 0, buf_size);
        uint32_t create_time_h = (uint32_t)(var_stat.create_time.tv_sec >> 32);
        uint32_t create_time_l =
            (uint32_t)(var_stat.create_time.tv_sec & 0xffffffff);
        uint32_t update_time_h = (uint32_t)(var_stat.update_time.tv_sec >> 32);
        uint32_t update_time_l =
            (uint32_t)(var_stat.update_time.tv_sec & 0xffffffff);
        sprintf(
            out_buf,
            "file stat "
            "dev_id=%d,size=%d,block_size=%d,block_count=%d,create_time=%d%d,"
            "update_time=%d%d\n",
            var_stat.dev_id, var_stat.size, var_stat.block_size,
            var_stat.block_count, var_stat.create_time.tv_sec >> 32,
            var_stat.create_time.tv_sec & 0xffffffff,
            var_stat.update_time.tv_sec >> 32,
            var_stat.update_time.tv_sec & 0xffffffff);
        write(STDOUT_FILENO, out_buf, strlen(out_buf));
    }
    close(fd);
    // ========================================================>
}

void test_dir() {
    init_tty();
    fd_t home = open("/home/.", O_RDONLY);
    dirent_t dirent;
    size_t buf_size = 512;
    char *out_buf = (char *)malloc(buf_size);
    memset(out_buf, 0, buf_size);
    while (readdir(home, &dirent) == 0) {
        sprintf(out_buf, "readdir dirent(name=%s, offset=%d, size=%d, type=%d)\n",dirent.name, dirent.offset, dirent.size, dirent.type);
        write(STDOUT_FILENO, out_buf, strlen(out_buf));
        memset(out_buf, 0, buf_size);
    }
    close(home);
}

void test_lfn() {
    init_tty();
    const char *file_name = "/home/test_long_long_long.txt";
    fd_t fd = open(file_name, O_CREAT | O_RDWR);
    if (fd < 0) {
        printf("open fail\n");
       return;
    }
    const char *buf = "test\nlong\nlong";
    ssize_t size = write(fd, buf, strlen(buf));
    if (size < 0) {
        printf("write fail\n");
        return;
    }
    lseek(fd, 0, 0);
    size_t buf_size = 512;
    char *out_buf = (char*)malloc(buf_size);
    memset(out_buf, 0, buf_size);
    size = read(fd, out_buf, buf_size);
    if (size < 0) {
        printf("read fail\n");
        return;
    }
    printf("read from %s, value=%s\n", file_name, out_buf);
    close(fd);
}

void test_device() {
    pid_t pid = fork();
    if (pid == 0) {
        // test_tty();
        // test_kbd();
        // test_disk();
        // test_dir();
        test_lfn();
    } else {
    }
}