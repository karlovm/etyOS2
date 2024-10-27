#include "partition.h"
#include "filesystem.h"
#include "print.h"
#include "port.h"


#define MBR_SIZE 512
#define PARTITION_TABLE_OFFSET 446
#define FAT32_PARTITION_TYPE 0x0C  // FAT32 with LBA support

void create_partition(uint32_t start_lba, uint32_t sector_count)
{
    uint8_t mbr[MBR_SIZE];

    // Read the MBR (sector 0)
    if (ata_read_sector(0, mbr) != 0)
    {
        print_str("Error: Unable to read MBR.");
        print_newline();
        return;
    }

    // Locate the first available slot in the MBR partition table (up to 4 slots)
    PartitionEntry *partition_table = (PartitionEntry *)(mbr + PARTITION_TABLE_OFFSET);

    for (int i = 0; i < 4; ++i)
    {
        if (partition_table[i].type == 0x00)
        {
            // Set up the new FAT32 partition
            partition_table[i].status = 0x00;               // Mark as inactive; can be set to bootable later
            partition_table[i].type = FAT32_PARTITION_TYPE; // FAT32 type
            partition_table[i].lba_first = start_lba;
            partition_table[i].sector_count = sector_count;
            break;
        }
    }

    // Write the updated MBR back to disk
    if (ata_write_sector(0, mbr) != 0)
    {
        print_str("Error: Unable to write MBR.");
        print_newline();
        return;
    }

    print_str("FAT32 Partition created successfully.");
    print_newline();
}



void display_partitions()
{
    uint8_t mbr[MBR_SIZE];

    // Read the MBR (sector 0)
    if (ata_read_sector(0, mbr) != 0)
    {
        print_str("Error: Unable to read MBR.");
        return;
    }

    PartitionEntry *partition_table = (PartitionEntry *)(mbr + PARTITION_TABLE_OFFSET);

    print_str("Partition Table:");
    print_newline();

    for (int i = 0; i < 4; ++i)
    {
        if (partition_table[i].type != 0x00)
        { // Check if partition entry is valid
            print_str("Partition ");
            print_int(i + 1);
            print_str(":");

            // Calculate partition size in MB
            uint32_t size_in_mb = (partition_table[i].sector_count * SECTOR_SIZE) / (1024 * 1024);

            print_str(" Start LBA: ");
            print_int(partition_table[i].lba_first);
            print_str(", Size: ");
            print_int(size_in_mb);
            print_str(" MB");
            print_newline();
        }
    }
}

void select_partition(int partition_number)
{
    uint8_t mbr[MBR_SIZE];

    // Read the MBR (sector 0)
    if (ata_read_sector(0, mbr) != 0)
    {
        print_str("Error: Unable to read MBR.");
        return;
    }

    PartitionEntry *partition_table = (PartitionEntry *)(mbr + PARTITION_TABLE_OFFSET);

    if (partition_number < 0 || partition_number >= 4 || partition_table[partition_number].type == 0x00)
    {
        print_str("Invalid or empty partition.");
        return;
    }

    print_str("Selected partition: ");
    print_int(partition_number);
    print_newline();
}

void format_fat32(uint32_t partition_start_lba, uint32_t partition_size)
{
    uint8_t sector[SECTOR_SIZE];

    // Zero out the sector buffer
    for (int i = 0; i < SECTOR_SIZE; ++i)
    {
        sector[i] = 0;
    }

    // Boot sector: setting up key FAT32 parameters
    sector[0x0B] = 0x00;
    sector[0x0C] = 0x02; // Bytes per sector (512 bytes)
    sector[0x0D] = 0x08; // Sectors per cluster
    sector[0x0E] = 0x20; // Reserved sectors (32)
    sector[0x10] = 0x02; // Number of FAT tables (2)
    sector[0x24] = 0x01; // Logical sectors per FAT (placeholder for now)

    // Signature
    sector[510] = 0x55;
    sector[511] = 0xAA;

    // Write the boot sector
    if (ata_write_sector(partition_start_lba, sector) != 0)
    {
        print_str("Error writing boot sector.");
        return;
    }

    // Write empty FAT table and root directory
    for (int i = 1; i < 10; ++i)
    {
        if (ata_write_sector(partition_start_lba + i, sector) != 0)
        {
            print_str("Error writing FAT table.");
            return;
        }
    }

    print_str("Formatted partition to FAT32.");
}
