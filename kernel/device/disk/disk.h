#if !defined(DISK_DISK_H)
#define DISK_DISK_H

#include "common/types/basic.h"
#define SECTOR_SIZE 512

#define PART_INFO_NAME_SIZE 32
struct _disk_t;
typedef struct {
    char name[PART_INFO_NAME_SIZE];
    enum {
        PART_UNVALID = 0x0,
        PART_FAT16 = 0x6,
    } type;

    int sector_start;
    int sector_count;
    struct _disk_t *disk;
} part_info_t;

#define DISK_NAME_SIZE 32
#define PART_COUNT 4
typedef struct _disk_t {
    char name[DISK_NAME_SIZE];
    uint32_t port_base;
    enum {
        MASTER,
        SLAVE
    } drive;

    size_t sector_size;
    int sector_count;

    part_info_t parts[PART_COUNT + 1];

} disk_t;

void init_disk();

#endif // DISK_DISK_H
