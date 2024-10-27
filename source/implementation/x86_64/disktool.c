#include "filesystem.h"
#include "partition.h"
#include "print.h"
#include "disktool.h"
#include "port.h"

#define MAX_DISKS 4
#define MB_TO_SECTORS(mb) ((mb * 1024 * 1024) / SECTOR_SIZE)
#define ATA_PRIMARY_IO_BASE 0x1F0
#define ATA_SECONDARY_IO_BASE 0x170
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_READ_SECTORS 0x20
#define ATA_CMD_WRITE_SECTORS 0x30

static DiskInfo available_disks[MAX_DISKS];
static int current_disk = -1;
static int num_disks = 0;

// Initialize the disk tool and scan for available disks
void init_disktool()
{
    num_disks = 0;
    print_str("Analyzing disks...");
    print_newline();
    uint16_t identify_buffer[256];

    for (int controller = 0; controller < 2; ++controller)
    {
        uint16_t io_base = (controller == 0) ? ATA_PRIMARY_IO_BASE : ATA_SECONDARY_IO_BASE;

        for (int drive = 0; drive < 2; ++drive)
        {
            outb(io_base + 6, 0xA0 | (drive << 4));
            outb(io_base + 7, ATA_CMD_IDENTIFY);

            uint8_t status = inb(io_base + 7);
            if (status == 0)
                continue;

            while ((inb(io_base + 7) & 0x80) != 0)
                ;

            for (int i = 0; i < 256; ++i)
            {
                identify_buffer[i] = inw(io_base);
            }

            DiskInfo *disk = &available_disks[num_disks];
            disk->controller = controller;
            disk->drive = drive;

            read_string_from_identify(identify_buffer, 27, 46, disk->model);

            uint32_t total_sectors = identify_buffer[60] | (identify_buffer[61] << 16);
            disk->size_mb = (total_sectors * SECTOR_SIZE) / (1024 * 1024);

            num_disks++;
        }
    }
    print_str("Found ");
    print_dec(num_disks);
    print_str(" disks");
    print_newline();
    print_newline();
}

// List all available disks
void list_disks()
{
    print_str("Available Disks:\n");
    print_newline();

    for (int i = 0; i < num_disks; i++)
    {
        print_str("[");
        print_int(i);
        print_str("] ");
        print_str(available_disks[i].model);
        print_str(" (");
        print_int(available_disks[i].size_mb);
        print_str(" MB)");
        print_newline();
    }
}

// Select a disk to work with
int select_disk(int disk_index)
{
    if (disk_index < 0 || disk_index >= num_disks)
    {
        print_str("Invalid disk index");
        print_newline();
        return -1;
    }

    current_disk = disk_index;
    print_str("Selected disk: ");
    print_str(available_disks[disk_index].model);
    print_newline();
    return 0;
}

// Create a new partition on the selected disk
int create_partition_mb(uint32_t size_mb, FileSystemType fs_type)
{
    if (current_disk == -1)
    {
        print_str("No disk selected");
        print_newline();
        return -1;
    }

    if (size_mb > available_disks[current_disk].size_mb)
    {
        print_str("Requested size exceeds disk capacity");
        print_newline();
        return -1;
    }

    uint32_t sector_count = MB_TO_SECTORS(size_mb);

    // Use the appropriate create_partition function based on filesystem type
    if (fs_type == FS_FAT32)
    {
        print_str("FAT32 PARTITIONS IS NOT RECOMMENDED");
        print_newline();
        create_fat32_partition(available_disks[current_disk].controller,
                              available_disks[current_disk].drive,
                              2048, sector_count);
    }
    else if (fs_type == FS_EXT4)
    {
        create_ext4_partition(available_disks[current_disk].controller,
                              available_disks[current_disk].drive,
                              2048, sector_count);
    }

    return 0;
}

// Format the entire selected disk as a single FAT32 partition
int format_disk(FileSystemType fs_type)
{
    if (current_disk == -1)
    {
        print_str("No disk selected");
        print_newline();
        return -1;
    }

    uint32_t total_size_mb = available_disks[current_disk].size_mb;

    if (total_size_mb < 32)
    {
        print_str("Disk too small for FAT32");
        print_newline();
        return -1;
    }

    uint32_t start_lba = 2048; // Standard starting sector for alignment
    uint32_t sector_count = MB_TO_SECTORS(total_size_mb) - start_lba;

    // Create the partition and format it
    if (fs_type == FS_FAT32)
    {
        print_str("FAT32 PARTITIONS IS NOT RECOMMENDED");
        print_newline();
        create_fat32_partition(available_disks[current_disk].controller,
                              available_disks[current_disk].drive,
                              2048, sector_count);
    }
    else if (fs_type == FS_EXT4)
    {
        create_ext4_partition(available_disks[current_disk].controller,
                              available_disks[current_disk].drive,
                              2048, sector_count);
    }
   
    return 0;
}

// Display filesystem type for each partition
void display_partition_info()
{
    if (current_disk == -1)
    {
        print_str("No disk selected");
        print_newline();
        return;
    }

    print_str("Disk: ");
    print_str(available_disks[current_disk].model);
    print_str(" (");
    print_int(available_disks[current_disk].size_mb);
    print_str(" MB)");
    print_newline();

    display_partitions(); // Use the function from partition.c
}

// Format a specific partition
int format_partition(int partition_index, FileSystemType fs_type)
{
    if (current_disk == -1)
    {
        print_str("No disk selected");
        print_newline();
        return -1;
    }

    uint8_t mbr[SECTOR_SIZE];
    if (ata_read_sector_disk(available_disks[current_disk].controller,
                             available_disks[current_disk].drive, 0, mbr) != 0)
    {
        print_str("Error reading MBR");
        print_newline();
        return -1;
    }

    PartitionEntry *partition_table = (PartitionEntry *)(mbr + PARTITION_TABLE_OFFSET);

    if (partition_table[partition_index].type == 0x00)
    {
        print_str("Partition does not exist");
        print_newline();
        return -1;
    }

    // Format using the appropriate function based on filesystem type
    if (fs_type == FS_FAT32)
    {
        return format_fat32(available_disks[current_disk].controller,
                            available_disks[current_disk].drive,
                            partition_table[partition_index].lba_first,
                            partition_table[partition_index].sector_count);
    }
    else if (fs_type == FS_EXT4)
    {
        return format_ext4(available_disks[current_disk].controller,
                           available_disks[current_disk].drive,
                           partition_table[partition_index].lba_first,
                           partition_table[partition_index].sector_count);
    }

    return -1;
}

// Delete a partition
int delete_partition(int partition_index)
{
    if (current_disk == -1)
    {
        print_str("No disk selected");
        print_newline();
        return -1;
    }

    if (partition_index < 0 || partition_index >= 4)
    {
        print_str("Invalid partition index");
        print_newline();
        return -1;
    }

    uint8_t mbr[SECTOR_SIZE];
    if (ata_read_sector_disk(available_disks[current_disk].controller,
                             available_disks[current_disk].drive, 0, mbr) != 0)
    {
        print_str("Error reading MBR");
        print_newline();
        return -1;
    }

    PartitionEntry *partition_table = (PartitionEntry *)(mbr + PARTITION_TABLE_OFFSET);

    if (partition_table[partition_index].type == 0x00)
    {
        print_str("Partition does not exist");
        print_newline();
        return -1;
    }

    // Clear the partition entry
    for (int i = 0; i < sizeof(PartitionEntry); i++)
    {
        ((uint8_t *)&partition_table[partition_index])[i] = 0;
    }

    if (ata_write_sector_disk(available_disks[current_disk].controller,
                              available_disks[current_disk].drive, 0, mbr) != 0)
    {
        print_str("Error writing MBR");
        print_newline();
        return -1;
    }

    print_str("Partition deleted successfully");
    print_newline();
    return 0;
}