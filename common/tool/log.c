#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../cpu/contrl.h"

#define LOG_PORT 0x3f8  // COM1

int init_log() {
    outb(LOG_PORT + 1, 0x00);  // 禁用所有中断
    outb(LOG_PORT + 3, 0x80);  // 使能 DLAB（设置波特率除数）
    outb(LOG_PORT + 0, 0x03);  // 将除数设置为 3（低字节）38400 波特率
    outb(LOG_PORT + 1, 0x00);  // (高字节)
    outb(LOG_PORT + 3, 0x03);  // 8 位，无奇偶校验，1 个停止位
    outb(LOG_PORT + 2, 0xC7);  // 启用 FIFO，清除它们，具有 14 字节阈值
    outb(LOG_PORT + 4, 0x0B);  // IRQ 启用，RTS/DSR 设置
    outb(LOG_PORT + 4, 0x1E);  // 设置环回模式，测试串口芯片
    outb(LOG_PORT + 0,
         0xAE);  // 测试串口芯片（发送字节0xAE并检查串口是否返回相同的字节）

    // 检查串口是否有错误（即：与发送的字节不同）
    if (inb(LOG_PORT + 0) != 0xAE) {
        return 1;
    }

    // 如果串行没有故障，则将其设置为正常操作模式
    // （启用 IRQ 且启用 OUT#1 和 OUT#2 位时不环回）
    outb(LOG_PORT + 4, 0x0F);
}

int is_transmit_empty() { return inb(LOG_PORT + 5) & 0x20; }

void write_serial(char a) {
    while (is_transmit_empty() == 0)
        ;

    outb(LOG_PORT, a);
}

void log_printf(const char* fmt, ...) {
    char str_buf[512];
    memset(str_buf, 0, 512);

    va_list args;
    va_start(args, fmt);
    vsprintf(str_buf, fmt, args);
    va_end(args);
    const char *p = str_buf;
    while (*p != '\0') {
        write_serial(*p);
        p++;
    }
}