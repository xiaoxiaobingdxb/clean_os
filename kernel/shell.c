#include "common/lib/stdio.h"
#include "common/lib/string.h"
#include "common/tool/math.h"
#include "include/device_model.h"
#include "include/syscall.h"
#include "io/std.h"
#include "memory/malloc/malloc.h"

void init_shell_std() {
    fd_t fd = open("/dev/tty0", TTY_OUT_LINE | TTY_OUT_R_N | TTY_OUT_ECHO);
    dup2(STDIN_FILENO, fd);
    dup2(STDOUT_FILENO, fd);
    dup2(STDERR_FILENO, fd);
}

extern void shell_main();
void init_shell() {
    init_shell_std();
    shell_main();
    // const char *file_path = "/home/shell.elf";
    // fd_t shell_fd = open(file_path, O_RDONLY);
    // if (shell_fd < 0) {
    //     printf("open %s fail\n", file_path);
    //     return;
    // }

    // int buf_size = 512;
    // byte_t *read_buf = malloc(buf_size);
    // int read_size = read(shell_fd, read_buf, buf_size);
}

typedef struct {
    const char *name;
    const char *usage;
    int (*func)(int argc, char **argv);
} cmd_t;

cmd_t cmd_list[];
cmd_t *find_cmd(char *cmd_str);

void show_prompt() { printf("shell >>"); }

#define ARG_COUNT_MAX 10
void shell_main() {
    char *buf = (char *)malloc(1024);
    char *save_ptr;
    while (true) {
        show_prompt();
        if (gets(buf) <= 0) {
            printf("gets error");
            continue;
        }
        char *cmd_str = strtok_r(buf, ";", &save_ptr);
        int argc = 0;
        char *argv[ARG_COUNT_MAX];
        while (cmd_str) {
            char *str_save_point;
            char *str = strtok_r(cmd_str, " ", &str_save_point);
            trim_left(str);
            char *cmd_name = str;
            argc = 0;
            while (str) {
                argv[argc++] = str;
                str = strtok_r(NULL, " ", &str_save_point);
            }
            cmd_t *cmd = find_cmd(cmd_name);
            if (!cmd) {
                printf("%s not found\n", cmd_name);
            } else {
                int return_code;
                if ((return_code = cmd->func(argc, argv)) != 0) {
                    fprintf(STDERR_FILENO, "call %s err, code=%d\n", cmd_name,
                            return_code);
                }
            }
            cmd_str = strtok_r(NULL, ";", &save_ptr);
        }
    }
}

#define declare_cmd_func(name) int do_##name(int argc, char **argv)
declare_cmd_func(clear);
declare_cmd_func(list);
declare_cmd_func(pwd);
declare_cmd_func(echo);
cmd_t cmd_list[] = {
    {
        .name = "clear",
        .usage = "clear the screen",
        .func = do_clear,
    },
    {
        .name = "ls",
        .usage = "list children for dir",
        .func = do_list,
    },
    {
        .name = "pwd",
        .usage = "show process working dir",
        .func = do_pwd,
    },
    {
        .name = "echo",
        .usage = "show message into screen",
        .func = do_echo,
    },
};
cmd_t *find_cmd(char *cmd_str) {
    size_t cmd_count = sizeof(cmd_list) / sizeof(cmd_t);
    for (cmd_t *cmd = cmd_list; cmd <= cmd_list + cmd_count; cmd++) {
        if (memcmp(cmd->name, cmd_str,
                   min(strlen(cmd->name), strlen(cmd_str))) == 0) {
            return cmd;
        }
    }
    return NULL;
}

int getopt(int argc, char **argv) {}

declare_cmd_func(clear) {
    printf("%c[2J", 0x1b);
    return 0;
}
declare_cmd_func(list) {
    char *path = malloc(256);
    if (path == NULL) {
        return -1;
    }
    if (argc == 1 || argc > 1 && argv[1][0] != '/') {
        sys_info info;
        pid_t pid = get_pid();
        if (pid < 0) {
            return -1;
        }
        if (sysinfo(pid, &info, 0) != 0) {
            return -1;
        }
        if (argc == 1) {
            memcpy(path, info.pwd, strlen(info.pwd));
        } else {
            sprintf(path, "%s/%s", info.pwd, argv[1]);
        }
    } else {
        memcpy(path, argv[1], strlen(argv[1]));
    }
    fd_t fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("ls: %s: No such file or directory\n", path);
        return -1;
    }
    dirent_t *dirent = malloc(sizeof(dirent_t));
    if (dirent == NULL) {
        return -1;
    }
    while (readdir(fd, dirent) == 0) {
        printf("%d %d %s\n", dirent->child_count, dirent->size, dirent->name);
    }
    close(fd);
    free(dirent);
    free(path);
    return 0;
}

declare_cmd_func(pwd) {
    sys_info info;
    pid_t pid = get_pid();
    if (pid < 0) {
        return -1;
    }
    if (sysinfo(pid, &info, 0) == 0) {
        printf("%s\n", info.pwd);
    }
    return 0;
}

declare_cmd_func(echo) { return 0; }