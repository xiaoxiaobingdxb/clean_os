// https://www.pcilookup.com/?ven=8086&dev=100E&action=submit

#include "../../device/pci/pci.h"
#include "../../interrupt/intr.h"
#include "../../interrupt/intr_no.h"
#include "../../memory/mem.h"
#include "common/cpu/contrl.h"
#include "common/cpu/mio.h"
#include "common/lib/string.h"
#include "common/tool/log.h"
#include "driver.h"
#include "../util.h"

#define VENDORID 0x8086
#define DEVICE_ID_LOW 0x1000
#define DEVICE_ID_HIGH 0x1028
pci_device_t *find_e1000_device() {
    pci_device_t *device;
    for (uint16_t device_id = DEVICE_ID_LOW; device_id <= DEVICE_ID_HIGH;
         device_id++) {
        device = pci_find_device(VENDORID, device_id);
        if (device != NULL) {
            break;
        }
    }
    return device;
}

typedef struct {
    char name[16];
    pci_device_t *device;
    uint32_t mem_base;
    bool eeprom;
    uint8_t mac[6];
} e1000_t;

e1000_t e1000;
#define E1000_EERD 0x14  // EEPROM Read EEPROM
static void e1000_eeprom_detect(e1000_t *e1000) {
    moutl(e1000->mem_base + E1000_EERD, 0x1);
    for (size_t i = 0; i < 1000; i++) {
        uint32_t val = minl(e1000->mem_base + E1000_EERD);
        if (val & 0x10) {
            e1000->eeprom = true;
        } else {
            e1000->eeprom = false;
        }
    }
}
// 读取只读存储器
uint16_t e1000_eeprom_read(e1000_t *desc, uint8_t addr) {
    uint32_t tmp;
    if (desc->eeprom) {
        moutl(desc->mem_base + E1000_EERD, 1 | (uint32_t)addr << 8);
        while (!((tmp = minl(desc->mem_base + E1000_EERD)) & (1 << 4)));
    } else {
        moutl(desc->mem_base + E1000_EERD, 1 | (uint32_t)addr << 2);
        while (!((tmp = minl(desc->mem_base + E1000_EERD)) & (1 << 1)));
    }
    return (tmp >> 16) & 0xFFFF;
}
void read_mac(e1000_t *desc) {
    e1000_eeprom_detect(desc);
    if (desc->eeprom) {
        for (int i = 0; i < 3; i++) {
            uint16_t value = e1000_eeprom_read(desc, i);
            desc->mac[i * 2] = value & 0xff;
            desc->mac[i * 2 + 1] = value >> 8;
        }
    } else {
        char *get_mac = (char *)desc->mem_base + 0x5400;
        for (int i = 0; i < 6; i++) {
            desc->mac[i] = get_mac[i];
        }
    }
    char *macStr = "11:22:33:44:55:66";
    mac2str(desc->mac, macStr);
    log_debug("e1000 mac:%s\n", macStr);
}
#define E1000_MAT0 0x5200   // Multicast Table Array 05200h-053FCh
#define E1000_MAT1 0x5400   // Multicast Table Array 05200h-053FCh
#define E1000_IMS 0xD0      // Interrupt Mask Set/Read
#define E1000_RDBAL 0x2800  // Receive Descriptor Base Address LOW
#define E1000_RDBAH 0x2804  // Receive Descriptor Base Address HIGH 64bit only
#define E1000_RDLEN 0x2808  // Receive Descriptor Length

#define IM_RXSEQ 1 << 3  // Receive Sequence Error.
// 到达接受描述符最小阈值，表示流量太大，接收描述符太少了，应该再多加一些，不过没有数据包丢失
#define IM_RXDMT0 1 << 4  // Receive Descriptor Minimum Threshold hit.
// 因为没有可用的接收缓冲区或因为PCI接收带宽不足，已经溢出，有数据包丢失
#define IM_RXO 1 << 6  // Receiver FIFO Overrun.
// 接收定时器中断
#define IM_RXT0 1 << 7  // Receiver Timer Interrupt.
// 连接状态变化，可以认为是网线拔掉或者插上
#define IM_LSC 1 << 2  // Link Status Change
// 传输队列为空
#define IM_TXQE 1 << 1  // Transmit Queue Empty.
// 传输描述符写回，表示有一个数据包发出
#define IM_TXDW 1 << 0  // Transmit Descriptor Written Back.
// 传输描述符环已达到传输描述符控制寄存器中指定的阈值。
#define IM_TXDLOW 1 << 15  // Transmit Descriptor Low Threshold hit
void e1000_reset(e1000_t *desc) {
    read_mac(desc);
    for (int i = E1000_MAT0; i < E1000_MAT1; i += 4) {
        moutl(desc->mem_base + i, 0);
    }
    moutl(desc->mem_base + E1000_IMS, 0);
    uint32_t value = 0;
    value = IM_RXT0 | IM_RXO | IM_RXDMT0 | IM_RXSEQ | IM_LSC;
    value |= IM_TXQE | IM_TXDW | IM_TXDLOW;
    moutl(desc->mem_base + E1000_IMS, value);
}

void e1000_handler(uint32_t intr_no) {
    log_debug("e1000_handler:%d\n", intr_no);
}

void set_intr(e1000_t *desc) {
    uint32_t intr = pci_interrupt(desc->device);

    log_debug("e1000 irq %d\n", intr);
    // assert(intr == IRQ_NIC);

    // 设置中断处理函数
    register_intr_handler(intr, e1000_handler);
}
void init_e1000() {
    pci_device_t *device = find_e1000_device();
    if (device == NULL) {
        log_err("e1000 not foud");
        return;
    }
    e1000_t *desc = &e1000;
    strcpy(desc->name, "e1000");
    desc->device = device;
    pci_enable_bus(device);
    pci_bar_t bar;
    pci_find_bar(device, &bar, PCI_BAR_TYPE_MEM);
    log_debug("e1000 io_bar=%d,size=%d\n", bar.io_base, bar.size);
    map_mem_direct(bar.io_base, bar.io_base, bar.size);
    desc->mem_base = bar.io_base;
    outb(desc->mem_base + 0x52, 0);
    e1000_reset(desc);
    set_intr(desc);
}