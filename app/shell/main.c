#include "common/lib/stdio.h"
#include "common/lib/string.h"
#include "common/tool/math.h"
#include "glibc/include/malloc.h"
// #include "glibc/include/stdio.h"
#include "glibc/io/std.h"
#include "include/device_model.h"
#include "include/syscall.h"

fd_t stdio_fd;

void init_shell_std() {
    stdio_fd = open("/dev/tty0", TTY_OUT_LINE | TTY_OUT_R_N | TTY_OUT_ECHO);
    dup2(STDIN_FILENO, stdio_fd);
    dup2(STDOUT_FILENO, stdio_fd);
    dup2(STDERR_FILENO, stdio_fd);
}

extern void shell_main();

void init_shell() {
    init_shell_std();
    shell_main();
}

int main(int argc, char **argv) {
    init_shell();
}

typedef struct {
    const char *name;
    const char *usage;

    int (*func)(int argc, char **argv);
} cmd_t;

cmd_t *find_cmd(char *cmd_str);

void show_prompt() { printf("shell >>"); }

error get_input_file_path(int argc, char **argv, char *path);

#define ARG_COUNT_MAX 10

void shell_main() {
    char *buf = (char *) malloc(1024);
    char *save_ptr;
    while (true) {
        show_prompt();
        if (gets(buf) <= 0) {
            printf("gets error");
            continue;
        }
        char *cmd_str = strtok_r(buf, ";", &save_ptr);
        int argc = 0;
        char *argv[ARG_COUNT_MAX + 1];
        while (cmd_str) {
            char *str_save_point;
            char *str = strtok_r(cmd_str, " ", &str_save_point);
            trim_left(str);
            char *cmd_name = str;
            argc = 0;
            while (str && argc < ARG_COUNT_MAX) {
                argv[argc++] = str;
                str = strtok_r(NULL, " ", &str_save_point);
            }
            argv[argc] = NULL;
            cmd_t *cmd = find_cmd(cmd_name);
            if (!cmd) {
                printf("%s not found\n", cmd_name);
            } else {
                bool has_redirect = false;
                fd_t redirect_fd;
                if (argc >= 3 &&
                    strcmp(argv[argc - 2], ">>") == 0) {  // redirect
                    char *path = malloc(256);
                    if (path) {
                        error err = 0;
                        char *tmp_argv[] = {argv[argc - 2], argv[argc - 1]};
                        if ((err = get_input_file_path(2, tmp_argv, path)) ==
                            0) {
                            redirect_fd = open(path, O_RDWR);
                            if (redirect_fd > 0) {
                                dup2(STDOUT_FILENO, redirect_fd);
                                has_redirect = true;
                            }
                        }
                        free(path);
                    }
                    argc -= 2;
                }

                int return_code;
                if ((return_code = cmd->func(argc, argv)) != 0) {
                    fprintf(STDERR_FILENO, "call %s err, code=%d\n", cmd_name,
                            return_code);
                }
                if (has_redirect) {
                    close(redirect_fd);
                    dup2(STDOUT_FILENO, stdio_fd);
                }
            }
            cmd_str = strtok_r(NULL, ";", &save_ptr);
        }
    }
}

#define declare_cmd_func(name)      \
    int do_##name(int argc, char *argv[])

declare_cmd_func(man);

declare_cmd_func(clear);

declare_cmd_func(list);

declare_cmd_func(pwd);

declare_cmd_func(free);

declare_cmd_func(echo);

declare_cmd_func(cat);

declare_cmd_func(date);

declare_cmd_func(exec);

cmd_t cmd_list[] = {
        {
                .name = "man",
                .usage = "show help for command",
                .func = do_man,
        },
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
                .name = "free",
                .usage = "show memory usage infoUsage:\n"
                         "free [options]\n"
                         "\n"
                         "Options:\n"
                         " -b                  show output in bytes\n"
                         " -k                  show output in kibibytes\n"
                         " -m                  show output in mebibytes\n"
                         " -g                  show output in gibibytes\n"
                         " -h                  show human-readable output\n"
                         //                 " -l, --lohi          show detailed low and high memory statistics\n"
                         //                 " -t, --total         show total for RAM + swap\n"
                         //                 " -v, --committed     show committed memory and commit limit\n"
                         //                 " -s N, --seconds N   repeat printing every N seconds\n"
                         //                 " -c N, --count N     repeat printing N times, then exit\n"
                         //                 " -w, --wide          wide output\n"
                         "\n"
//                 "     --help     display this help and exit\n"
//                 " -V, --version  output version information and exit",
                ,
                .func = do_free,
        },
        {
                .name = "echo",
                .usage = "show message into screen",
                .func = do_echo,
        },
        {
                .name = "cat",
                .usage = "print file content",
                .func = do_cat,
        },
        {
                .name = "date",
                .usage = "display or set date and time",
                .func = do_date,
        },
        {
                .name = "exec",
                .usage = "execute a program",
                .func = do_exec,
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

declare_cmd_func(man) {
    if (argc <= 1) {
        printf("please input the command you want to know\n");
        return -1;
    }
    trim(argv[1]);
    char *command = argv[1];
    cmd_t *cmd = NULL;
    size_t cmd_list_count = sizeof(cmd_list) / sizeof(cmd_t);
    for (int i = 0; i < cmd_list_count; i++) {
        cmd_t find_cmd = cmd_list[i];
        if (memcmp(find_cmd.name, command, strlen(find_cmd.name)) == 0) {
            cmd = &find_cmd;
            break;
        }
    }
    if (cmd == NULL) {
        printf("command %s not found\n", command);
        return -1;
    }
    printf("%s\n", cmd->usage);
    return 0;
}

declare_cmd_func(clear) {
    printf("%c[2J", 0x1b);
    return 0;
}

error get_input_file_path(int argc, char **argv, char *path) {
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
    return 0;
}

declare_cmd_func(list) {
    char *path = malloc(256);
    if (path == NULL) {
        goto finallly;
    }
    error err = 0;
    if ((err = get_input_file_path(argc, argv, path)) != 0) {
        goto finallly;
    }
    fd_t fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("ls: %s: No such file or directory\n", path);
        err = -1;
        goto finallly;
    }
    dirent_t *dirent = malloc(sizeof(dirent_t));
    if (dirent == NULL) {
        err = -1;
        goto finallly;
    }
    while (readdir(fd, dirent) == 0) {
        printf("%d %d %s\n", dirent->child_count, dirent->size, dirent->name);
    }
    finallly:
    if (fd >= 0) {
        close(fd);
    }
    if (dirent != NULL) {
        free(dirent);
    }
    if (path != NULL) {
        free(path);
    }
    return err;
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

#define byte_scale  1024

uint32_t readable_unit(uint32_t value, char *unit) {
    if (value < byte_scale) {
        sprintf(unit, "byte");
        return value;
    } else if (value < byte_scale * byte_scale) {
        sprintf(unit, "kb");
        return value / byte_scale;
    } else if (value < byte_scale * byte_scale * byte_scale) {
        sprintf(unit, "mb");
        return value / byte_scale / byte_scale;
    }
    sprintf(unit, "gb");
    return value / byte_scale / byte_scale / byte_scale;
}

declare_cmd_func(free) {
    sys_info info;
    pid_t pid = get_pid();
    if (pid < 0) {
        return -1;
    }
    if (sysinfo(pid, &info, SYS_INFO_MEM) != 0) {
        return -1;
    }
    uint32_t kernel_total = info.mem_info.kernel_total;
    uint32_t user_total = info.mem_info.user_total;
    uint32_t kernel_phy = info.mem_info.kernel_phy_mem_used;
    uint32_t kernel_used = info.mem_info.kernel_mem_used;
    uint32_t user_phy = info.mem_info.user_phy_mem_used;
    uint32_t user_used = info.mem_info.user_mem_used;
    char *unit = "byte";
    if (argc > 1) {
        size_t strl = strlen(argv[1]) + 1;
        char *option = malloc(strl);
        memset(option, 0, strl);
        memcpy(option, argv[1], strl);
        trim(option);
        if (strlen(option) < 2) {
            printf("free: invalid option %s\n", option);
            cmd_t *cmd = find_cmd("free");
            if (cmd == NULL) {
                return -1;
            }
            printf("%s\n", cmd->usage);
            return -1;
        }
        uint32_t scale = 1;
        if (option[1] == 'h') {
            char *unit1 = "byte";
            char *unit2 = "byte";
            printf("               total        used\n"/*        free      shared  buff/cache   available*/);
            kernel_total = readable_unit(kernel_total, unit1);
            kernel_phy = readable_unit(kernel_phy, unit2);
            printf("kernel_phy     %d%s         %d%s\n", kernel_total, unit1, kernel_phy, unit2);
            kernel_used = readable_unit(kernel_used, unit2);
            printf("kernel_vir     %d%s         %d%s\n", kernel_total, unit1, kernel_used, unit2);
            user_total = readable_unit(user_total, unit1);
            user_phy = readable_unit(user_phy, unit2);
            printf("user_phy       %d%s         %d%s\n", user_total, unit1, user_phy, unit2);
            user_used = readable_unit(user_used, unit2);
            printf("user_vir       %d%s         %d%s\n", user_total, unit1, user_used, unit2);
            return 0;
        } else {
            switch (option[1]) {
                case 'k':
                    scale = byte_scale;
                    unit = "kb";
                    break;
                case 'm':
                    unit = "mb";
                    scale = byte_scale * byte_scale;
                    break;
                case 'g':
                    unit = "gb";
                    scale = byte_scale * byte_scale * byte_scale;
                    break;
            }
            kernel_total /= scale;
            kernel_phy /= scale;
            kernel_used /= scale;
            user_total /= scale;
            user_phy /= scale;
            user_used /= scale;
        }
    }
    printf("               total        used\n"/*        free      shared  buff/cache   available*/);
    printf("kernel_phy     %d%s         %d%s\n", kernel_total, unit, kernel_phy, unit);
    printf("kernel_vir     %d%s         %d%s\n", kernel_total, unit, kernel_used, unit);
    return 0;
}

declare_cmd_func(echo) {
    for (int i = 1; i < argc - 1; i++) {
        printf("%s ", argv[i]);
    }
    if (argc > 1) {
        printf("%s", argv[argc - 1]);
    }
    printf("\n");
    return 0;
}

declare_cmd_func(cat) {
    char *path = malloc(256);
    if (path == NULL) {
        return -1;
    }
    error err = 0;
    if ((err = get_input_file_path(argc, argv, path)) != 0) {
        goto finallly;
    }
    fd_t fd = open(path, O_RDONLY);
    if (fd < 0) {
        err = -1;
        goto finallly;
    }
    const int buf_size = 512;
    byte_t *read_buf = malloc(buf_size);
    memset(read_buf, 0, buf_size);
    ssize_t read_size;
    while ((read_size = read(fd, read_buf, buf_size)) == buf_size) {
        printf("%s", read_buf);
        memset(read_buf, 0, buf_size);
    }
    if (read_size > 0) {
        printf("%s", read_buf);
    }
    printf("\n");
    free(read_buf);
    finallly:
    if (path != NULL) {
        free(path);
    }
    if (fd >= 0) {
        close(fd);
    }
    return err;
}

declare_cmd_func(date) {
    timespec_t timespec;
    memset(&timespec, 0, sizeof(timespec));
    timestamp(&timespec);
    printf("unix:%l %l\n", timespec.tv_sec, timespec.tv_nsec);
    datetime_t datetime;
    mkdatetime(timespec.tv_sec, &datetime);
    printf("%d year %d month %d day %d hour %d minute %d second\n",
           datetime.year, datetime.month, datetime.day, datetime.hour,
           datetime.minute, datetime.second);
    pid_t pid = get_pid();
    if (pid >= 0) {
        sys_info info;
        if (!sysinfo(pid, &info, SYS_INFO_CPU_INFO)) {
            printf("cpu:%l\n", info.cpu_info.rdtscp);
        }
    }
    return 0;
}

declare_cmd_func(exec) {
    char *path = malloc(256);
    if (path == NULL) {
        return -1;
    }
    error err = 0;
    if ((err = get_input_file_path(argc, argv, path)) != 0) {
        free(path);
        return err;
    }
    pid_t pid = fork();
    char **heap_argv = malloc((argc + 1) * sizeof(char *));
    for (int i = 0; i < argc + 1; i++) {
        heap_argv[i] = argv[i];
    }
    if (pid == 0) {
        if ((err = execve(path, heap_argv, NULL)) != 0) {
            printf("not found program file %s\n", path);
            goto finally;
        }
    }
    int status;
    wait(&status);
    finally:
    if (path) {
        free(path);
    }
    return err;
}