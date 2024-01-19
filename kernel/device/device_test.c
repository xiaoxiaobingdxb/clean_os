#include "../io/std.h"
#include "../memory/malloc/malloc.h"
#include "../syscall/syscall_user.h"
#include "common/lib/stdio.h"
#include "common/lib/string.h"

void test_tty() {
    init_std();
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
    init_std();
    int buf_size = 32;
    char *buf = mmap_anonymous(buf_size);
    ssize_t count = 0;
    size_t get_size = 0;
    while ((count = read(STDIN_FILENO, (void *)buf, buf_size)) > 0) {
        get_size = count;
    }
}

void test_disk() {
    init_std();
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

void read_file(const char *dir, const char *name) {
    char *path_buf = malloc(strlen(dir) + strlen(name) + 1);
    sprintf(path_buf, "%s/%s", dir, name);
    fd_t fd = open(path_buf, O_RDONLY);
    if (fd < 0) {
        printf("open file %s err", path_buf);
        return;
    }
    printf("read from file %s:\n", path_buf);
    const size_t buf_size = 256;
    char *read_buf = malloc(buf_size);
    while (true) {
        ssize_t read_size = read(fd, read_buf, buf_size);
        if (read_size < 0) {
            printf("read file %s err", path_buf);
            break;
        }
        printf(read_buf);
        if (read_size < buf_size) {
            break;
        }
    }
    free(read_buf);
    free(path_buf);
    close(fd);
}

void test_dir() {
    init_std();
    char *dirs[] = {"/home/.", "/home/sub_dir", "/home/long_long_long_sub"};
    for (int i = 0; i < sizeof(dirs) / sizeof(char *); i++) {
        fd_t home = open(dirs[i], O_RDONLY);
        if (home < 0) {
            continue;
        }
        dirent_t dirent;
        size_t buf_size = 512;
        char *out_buf = (char *)malloc(buf_size);
        memset(out_buf, 0, buf_size);
        while (readdir(home, &dirent) == 0) {
            sprintf(out_buf,
                    "readdir from %s dirent(name=%s, offset=%d, size=%d, "
                    "type=%d)\n",
                    dirs[i], dirent.name, dirent.offset, dirent.size,
                    dirent.type);
            write(STDOUT_FILENO, out_buf, strlen(out_buf));
            memset(out_buf, 0, buf_size);
            // if (dirent.type == FILE_FILE && i > 0) {
            //     read_file(dirs[i], dirent.name);
            // }
        }
        close(home);
    }
}

void test_sub_dir() {
    init_std();
    fd_t sub_dir = open("/home/sub_dir", O_RDONLY);
    if (sub_dir >= 0) {
        dirent_t dirent;
        while ((readdir(sub_dir, &dirent)) == 0) {
            printf(
                "readdir from %s dirent(name=%s, offset=%d, size=%d, "
                "type=%d)\n",
                "/home/sub_dir", dirent.name, dirent.offset, dirent.size,
                dirent.type);
        }
    }

    fd_t fd = open("/home/sub_dir/long_long_long_sub/test.txt", O_RDONLY);
    if (fd < 0) {
        printf("not found");
        return;
    }
    char *read_buf = (char *)malloc(256);
    ssize_t read_size = read(fd, read_buf, 256);
    printf("read:%s\n", read_buf);
    free(read_buf);
    close(fd);
}

void test_lfn() {
    init_std();
    const char *file_name = "/home/test_long_long_long.txt";
    fd_t fd = open(file_name, O_CREAT | O_RDWR | O_APPEND);
    if (fd < 0) {
        printf("open fail\n");
        return;
    }
    const char *buf = "test\nlong\nlong\n";
    ssize_t size = write(fd, buf, strlen(buf));
    if (size < 0) {
        printf("write fail\n");
        return;
    }
    lseek(fd, 0, 0);
    size_t buf_size = 512;
    char *out_buf = (char *)malloc(buf_size);
    memset(out_buf, 0, buf_size);
    size = read(fd, out_buf, buf_size);
    if (size < 0) {
        printf("read fail\n");
        return;
    }
    printf("read from %s, value=%s\n", file_name, out_buf);
    close(fd);
}

void test_ext2() {
    init_std();
    const char *file_name = "/ext2/test.txt";
    fd_t fd = open(file_name, O_RDWR);
    if (fd < 0) {
        printf("open fail\n");
        return;
    }
    char *read_buf = malloc(512);
    ssize_t read_size = 0;
    while ((read_size = read(fd, read_buf, 512)) > 0) {
        printf("read from %s:%s\n", file_name, read_buf);
    }
    char *write_buf = "ttt\n";
    ssize_t write_size = write(fd, write_buf, strlen(write_buf));
    printf("write size:%d\n", write_size);
    free(read_buf);
    close(fd);
}

void test_device() {
    // test_tty();
    // test_kbd();
    // test_disk();
    // test_dir();
    // test_lfn();
    // test_sub_dir();
    test_ext2();
}