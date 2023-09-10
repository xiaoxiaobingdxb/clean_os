#ifndef IO_DISK_H
#define IO_DISK_H
#include "../types/basic.h"

#define SECTOR_SIZE 512

void read_disk(int sector, int sector_count, uint8_t * buf);

#endif