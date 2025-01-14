#ifndef PCI_PCI_H
#define PCI_PCI_H
#include "common/lib/list.h"
#include "common/types/basic.h"
typedef struct {
    list_node node;
    uint8_t bus;
    uint8_t dev;
    uint8_t func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t revision;
    uint32_t class_code;
} pci_device_t;
typedef struct {
    uint32_t io_base;
    uint32_t size;
} pci_bar_t;
static list pci_device_list;
void init_pci();
pci_device_t* pci_find_device(uint16_t vendor_id, uint16_t device_id);
void pci_enable_bus(pci_device_t* device);
#define PCI_BAR_TYPE_MEM 0
#define PCI_BAR_TYPE_IO 1
error pci_find_bar(pci_device_t* device, pci_bar_t *bar, int type);
uint8_t pci_interrupt(pci_device_t *device);
#endif  // PCI_PCI_H