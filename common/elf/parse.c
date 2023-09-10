#include "elf.h"
uint32_t reload_elf_file(uint8_t *file_buffer) {
  // 读取的只是ELF文件，不像BIN那样可直接运行，需要从中加载出有效数据和代码
  // 简单判断是否是合法的ELF文件
  Elf32_Ehdr *elf_hdr = (Elf32_Ehdr *)file_buffer;
  if ((elf_hdr->e_ident[0] != ELF_MAGIC) || (elf_hdr->e_ident[1] != 'E') ||
      (elf_hdr->e_ident[2] != 'L') || (elf_hdr->e_ident[3] != 'F')) {
    return 0;
  }

  // 然后从中加载程序头，将内容拷贝到相应的位置
  for (int i = 0; i < elf_hdr->e_phnum; i++) {
    Elf32_Phdr *phdr = (Elf32_Phdr *)(file_buffer + elf_hdr->e_phoff) + i;
    if (phdr->p_type != PT_LOAD) {
      continue;
    }

    // 全部使用物理地址，此时分页机制还未打开
    uint8_t *src = file_buffer + phdr->p_offset;
    uint8_t *dest = (uint8_t *)phdr->p_paddr;
    for (int j = 0; j < phdr->p_filesz; j++) {
      *dest++ = *src++;
    }

    // memsz和filesz不同时，后续要填0
    dest = (uint8_t *)phdr->p_paddr + phdr->p_filesz;
    for (int j = 0; j < phdr->p_memsz - phdr->p_filesz; j++) {
      *dest++ = 0;
    }
  }

  return elf_hdr->e_entry;
}