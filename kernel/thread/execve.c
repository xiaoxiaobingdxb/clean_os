#include "../fs/fs.h"
#include "../memory/malloc/mallocator.h"
#include "./process.h"
#include "common/elf/elf.h"
#include "common/lib/string.h"
#include "include/syscall.h"
#include "thread.h"

uint32_t load_elf(uint8_t *file_buffer) {
    Elf32_Ehdr *elf_hdr = (Elf32_Ehdr *)file_buffer;
    if ((elf_hdr->e_ident[0] != ELF_MAGIC) || (elf_hdr->e_ident[1] != 'E') ||
        (elf_hdr->e_ident[2] != 'L') || (elf_hdr->e_ident[3] != 'F')) {
        return 0;
    }

    for (int i = 0; i < elf_hdr->e_phnum; i++) {
        Elf32_Phdr *phdr = (Elf32_Phdr *)(file_buffer + elf_hdr->e_phoff) + i;
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        if (!process_mmap((void *)phdr->p_vaddr, phdr->p_memsz,
                          PROT_READ | PROT_WRITE, MAP_ANONYMOUS, 0, 0)) {
            return 0;
        }
        uint8_t *dest = (uint8_t *)phdr->p_vaddr;
        uint8_t *src = file_buffer + phdr->p_offset;
        for (int j = 0; j < phdr->p_filesz; j++) {
            *(dest + j) = *(src + j);
        }

        dest = (uint8_t *)phdr->p_paddr + phdr->p_filesz;
        for (int j = 0; j < phdr->p_memsz - phdr->p_filesz; j++) {
            *dest++ = 0;
        }
    }

    return elf_hdr->e_entry;
}

extern void start_process(void *p_func);
void exec(uint8_t *elf_buf, char *const argv[], char *const envp[]) {
    uint32_t entry = load_elf(elf_buf);
    if (!entry) {
        return;
    }
    kernel_mallocator.free(elf_buf);
    start_process((void *)entry);
}

void rename_process_name(const char *filename) {
    task_struct *cur = cur_thread();
    const char *name = strrchr(filename, '/') + 1;
    memset(cur->name, 0, TASK_NAME_LEN);
    memcpy(cur->name, name, min(strlen(name), TASK_NAME_LEN));
}

int process_execve(const char *filename, char *const argv[],
                   char *const envp[]) {
    fd_t fd = sys_open(filename, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    stat_t stat;
    if (sys_fstat(fd, &stat)) {
        return -1;
    }
    size_t elf_size = stat.size;
    uint8_t *elf_buf = kernel_mallocator.malloc(elf_size);
    if (!elf_buf) {
        return -1;
    }
    ssize_t total = 0;
    const int buf_size = 512;
    byte_t *read_buf = kernel_mallocator.malloc(buf_size);
    ssize_t read_size;
    while (total < elf_size &&
           (read_size = sys_read(fd, read_buf, buf_size)) == buf_size) {
        memcpy(elf_buf + total, read_buf, read_size);
        total += read_size;
        memset(read_buf, 0, buf_size);
    }
    if (read_size > 0) {
        memcpy(elf_buf + total, read_buf, read_size);
        total += read_size;
    }
    kernel_mallocator.free(read_buf);
    sys_close(fd);
    rename_process_name(filename);
    exec(elf_buf, argv, envp);
    return 0;
}