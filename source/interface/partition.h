#ifndef PARTITION_H
#define PARTITION_H

#include <stdint.h>

#define MBR_SIZE 512
#define PARTITION_TABLE_OFFSET 0x1BE
#define PARTITION_ENTRY_SIZE 16
#define FAT32_PARTITION_TYPE 0x0C

typedef struct {
    uint8_t status;         // 0x00 = Inactive, 0x80 = Active (bootable)
    uint8_t chs_first[3];   // CHS address of the first absolute sector in partition
    uint8_t type;           // Partition type
    uint8_t chs_last[3];    // CHS address of the last absolute sector in partition
    uint32_t lba_first;     // LBA of the first absolute sector in partition
    uint32_t sector_count;  // Number of sectors in partition
} PartitionEntry;

// Function declarations
void create_partition(uint32_t start_lba, uint32_t sector_count);
void select_partition(int partition_number);
void format_fat32(uint32_t partition_start_lba, uint32_t partition_size);
// Function declaration to display partition information
void display_partitions();

#endif
