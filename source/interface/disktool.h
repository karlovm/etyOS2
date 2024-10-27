// disktool.h
#ifndef DISKTOOL_H
#define DISKTOOL_H

#include <stdint.h>



// Other constants
#define SECTOR_SIZE            512
#define MBR_SIZE              SECTOR_SIZE
#define PARTITION_TABLE_OFFSET 0x1BE
#define FAT32_PARTITION_TYPE  0x0C    // FAT32 LBA type

// Disk tool functions
void init_disktool(void);
void list_disks(void);
int select_disk(int disk_index);
int create_partition_mb(uint32_t size_mb);
void display_partition_info(void);
int delete_partition(int partition_index);


// Disk information structure
typedef struct {
    int controller;           // 0 = primary, 1 = secondary
    int drive;               // 0 = master, 1 = slave
    uint64_t size_mb;        // Size in megabytes
    char model[41];          // Model name
} DiskInfo;

#endif // DISKTOOL_H