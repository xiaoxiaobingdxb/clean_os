#include "pci.h"

#include "common/cpu/contrl.h"
#include "common/types/basic.h"
#include "glibc/include/malloc.h"
#include "common/tool/log.h"
#include "./util.h"

#pragma pack(1)
typedef struct {
    uint8_t io_space : 1;
    uint8_t memory_space : 1;
    uint8_t bus_master : 1;
    uint8_t special_cycles : 1;
    uint8_t memory_write_invalidate_enable : 1;
    uint8_t vga_palette_snoop : 1;
    uint8_t parity_error_response : 1;
    uint8_t RESERVED : 1;
    uint8_t serr : 1;
    uint8_t fast_back_to_back : 1;
    uint8_t interrupt_disable : 1;
    uint8_t RESERVED : 5;
} pci_command_t;

typedef struct {
    uint8_t RESERVED : 3;
    uint8_t interrupt_status : 1;
    uint8_t capabilities_list : 1;
    uint8_t mhz_capable : 1;
    uint8_t RESERVED : 1;
    uint8_t fast_back_to_back : 1;
    uint8_t master_data_parity_error : 1;
    uint8_t devcel : 2;
    uint8_t signaled_target_abort : 1;
    uint8_t received_target_abort : 1;
    uint8_t received_master_abort : 1;
    uint8_t signaled_system_error : 1;
    uint8_t detected_parity_error : 1;
} pci_status_t;
#pragma pack()

#define PCI_CONF_ADDR 0xCF8
#define PCI_CONF_DATA 0xCFC
#define PCI_BAR_NR 6
#define PCI_ADDR(bus, dev, func, addr) ( \
    (uint32_t)(0x80000000) |             \
    ((bus & 0xff) << 16) |               \
    ((dev & 0x1f) << 11) |               \
    ((func & 0x7) << 8) |                \
    addr)
#define PCI_CONF_VENDOR 0X0  
#define PCI_CONF_DEVICE 0X2   
#define PCI_CONF_COMMAND 0x4  
#define PCI_CONF_STATUS 0x6   
#define PCI_CONF_REVISION 0x8 

#define PCI_COMMAND_MASTER 0x0004      // Enable bus mastering

#define PCI_CONF_BASE_ADDR0 0x10


uint32_t pci_inl(uint8_t bus, uint8_t dev, uint8_t func, uint8_t addr) {
    outl(PCI_CONF_ADDR, PCI_ADDR(bus, dev, func, addr));
    return inl(PCI_CONF_DATA);
}
void pci_outl(uint8_t bus, uint8_t dev, uint8_t func, uint8_t addr, uint32_t value) {
    // outl(PCI_CONF_ADDR, PCI_ADDR(bus, dev, func, addr));
    outl(PCI_CONF_ADDR, ((uint32_t) 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | addr));
    outl(PCI_CONF_DATA, value);
}
void pci_check_device(int bus, int dev) {
    uint32_t value = 0;
    for (uint8_t func = 0; func < 8; func++) {
        value = pci_inl(bus, dev, func, PCI_CONF_VENDOR);
        uint16_t vendor_id = value & 0xffff;
        if (vendor_id == 0 || vendor_id == 0xffff) {
            return;
        }
        pci_device_t *device = (pci_device_t*)kernel_mallocator.malloc(sizeof(pci_device_t));
        pushr(&pci_device_list, &device->node);
        device->bus = bus;
        device->dev = dev;
        device->func = func;
        device->vendor_id = vendor_id;
        device->device_id = value >> 16;

        value = pci_inl(bus, dev, func, PCI_CONF_COMMAND);
        pci_command_t *cmd = (pci_command_t*)&value;
        pci_status_t *status = (pci_status_t*)(&value + 2);

        value = pci_inl(bus, dev, func, PCI_CONF_REVISION);
        device->class_code = value >> 8;
        device->revision = value & 0xff;
        log_debug("pci %d:%d.%d %d:%d %s\n",
        device->bus, device->dev, device->func,
        device->vendor_id, device->device_id,
         pci_classname(device->class_code));
    }
}
void pci_enum_device() {
    for (int bus = 0; bus < 256; bus++) {
        for (int dev = 0; dev < 32; dev++) {
            pci_check_device(bus, dev);
        }
    }
}
void init_pci() {
    init_list(&pci_device_list);
    pci_enum_device();
}

bool find_device_by_vendor_id_device_id(list_node *_node, void *arg) {
    uint16_t *args = (uint16_t*)arg;
    uint16_t vendor_id = args[0];
    uint16_t device_id = args[1];
    
    pci_device_t *device = tag2entry(pci_device_t, node, _node);
    if (device != 0 && device->vendor_id == vendor_id
        && device->device_id == device_id
    ) {
        return false;
    }
    return true;
}
pci_device_t* pci_find_device(uint16_t vendor_id, uint16_t device_id) {
    uint16_t arg[] = {vendor_id, device_id};
    list_node *device_node = list_find(&pci_device_list, find_device_by_vendor_id_device_id,(void*)arg);
    if (device_node) {
        return tag2entry(pci_device_t, node, device_node);
    }
    return NULL;
}

void pci_enable_bus(pci_device_t* device) {
    uint32_t value;

    uint32_t data = pci_inl(device->bus, device->dev, device->func, PCI_CONF_COMMAND);
    data |= PCI_COMMAND_MASTER;
    pci_outl(device->bus, device->dev, device->func, PCI_CONF_COMMAND, data);
}

#define PCI_BAR_NR 6
#define PCI_BAR_IO_MASK (~0x3)
#define PCI_BAR_MEM_MASK (~0xF)
static uint32_t pci_size(uint32_t base, uint32_t mask){
    uint32_t size = mask & base;
    size = ~size + 1;
    return size;
}
error pci_find_bar(pci_device_t* device, pci_bar_t *bar, int type) {
    for (int i = 0; i < PCI_BAR_NR; i++) {
        uint8_t addr = PCI_CONF_BASE_ADDR0 + (i << 2);
        uint32_t value = pci_inl(device->bus, device->dev, device->func, addr);
        pci_outl(device->bus, device->dev, device->func, addr, -1);
        uint32_t len = pci_inl(device->bus, device->dev, device->func, addr);
        pci_outl(device->bus, device->dev, device->func, addr, value);

        if (value == 0) {
            continue;
        }
        if (len <= 0) {
            continue;
        }
        if (value < 0) {
            value = 0;
        }
        if ((value & 1) && type == PCI_BAR_TYPE_IO) {
            bar->io_base = value & PCI_BAR_IO_MASK;
            bar->size = pci_size(len, PCI_BAR_IO_MASK);
            return 0;
        }
        if (!(value & 1) && type == PCI_BAR_TYPE_MEM) {
            bar->io_base = value & PCI_BAR_MEM_MASK;
            bar->size = pci_size(len, PCI_BAR_MEM_MASK);
            return 0;
        }
    }
    return -1;
}

#define PCI_CONF_INTERRUPT 0x3C
#define IRQ0_BASE           0x20
uint8_t pci_interrupt(pci_device_t *device)
{
    uint32_t data = pci_inl(device->bus, device->dev, device->func, PCI_CONF_INTERRUPT);
    return (data & 0xff) + IRQ0_BASE;
}