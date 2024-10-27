#ifndef PARTITION_H
#define PARTITION_H

#include <stdint.h>

#define MBR_SIZE 512
#define PARTITION_TABLE_OFFSET 0x1BE
#define PARTITION_ENTRY_SIZE 16
#define FAT32_PARTITION_TYPE 0x0C

// Partition table entry structure
typedef struct __attribute__((packed)) {
    uint8_t  status;           // 0x80 = bootable, 0x00 = not bootable
    uint8_t  start_head;       // Starting head
    uint16_t start_sector;     // Starting sector
    uint8_t  type;            // Partition type
    uint8_t  end_head;        // Ending head
    uint16_t end_sector;      // Ending sector
    uint32_t lba_first;       // First LBA sector
    uint32_t sector_count;    // Number of sectors
} PartitionEntry;



// Function declarations
void create_partition(uint32_t start_lba, uint32_t sector_count);
void select_partition(int partition_number);
void format_fat32(uint32_t partition_start_lba, uint32_t partition_size);
// Function declaration to display partition information
void display_partitions();

#endif
